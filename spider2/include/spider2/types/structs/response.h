//
// Created by jhrub on 11.12.2022.
//

#pragma once

#include "../platform.h"
#include "../utils/async_file_io.h"
#include "../utils/ignore_result.h"

#include "byte_range.h"
#include "request_error_code.h"
#include "spider2/types/utils/overload.h"
#include <fmt/format.h>
#include <numeric>

namespace spider2
{

   struct with_no_response
   {
   };

   auto copy_headers_to_message(http::fields const &headers, auto &message)
   {
      for (auto &val : headers)
      {
         if (http::field name = val.name(); name != http::field::unknown)
         {
            message.set(name, val.value());
         }
         else
         {
            message.set(val.name_string(), val.value());
         }
      }
   }

   struct file_response : public http::response<http::empty_body>
   {
      file_handle file;

      struct response_settings
      {
         /// file handle size used for ranges validation length when not specified
         std::uint64_t file_size = 0;

         /// offset  (default 0)
         std::uint64_t offset = 0;

         /// length is not used when ranges are present (default max - use file_size value)
         std::uint64_t length = std::numeric_limits<std::uint64_t>::max();

         /// false - means that request is conditional or by HEAD method - and we should not return the body
         bool write_body = true;

         /// true - marks file for deletion
         bool delete_file_after_write = false;
      };

      response_settings settings = {};
   };

   struct file_byte_ranges_response : public http::response<http::empty_body>
   {
      file_handle file;

      struct response_settings
      {
         /**
          *  Indicates that request is contains Range header, it is responsibility the caller that ranges are valid.
          *  Caller MUST ensure that response is sent with http::status::partial_content
          *  Requires size() >= 1
          **/
         byte_ranges ranges = {};

         /// boundary is required when ranges.size() > 1
         string boundary;
         std::uint64_t file_size = 0;
         bool write_body = true;
      };

      response_settings settings = {};
   };

   using response_type = std::variant<http::response<http::empty_body>, http::response<http::string_body>,
                                      file_response, file_byte_ranges_response, with_no_response>;

   const static http::response<http::empty_body> test_obj{};

   struct response
   {
      response_type message = {};
      optional<std::chrono::steady_clock::duration> keep_alive = {};

      inline auto async_write(tcp::socket &socket) noexcept -> io::awaitable<tuple<error_code, size_t>>
      {
         co_return (co_await std::visit(
            overload{
               [](with_no_response &) noexcept -> io::awaitable<tuple<error_code, size_t>>
               {
                  co_return tuple<error_code, size_t>{{}, {}};
               },
               [&](file_response &response) noexcept -> io::awaitable<tuple<error_code, size_t>>
               {
                  apply_keep_alive(response);

                  auto &settings = response.settings;
                  if (settings.delete_file_after_write)
                  {
                     IGNORE_RESULT(async_file_io::delete_file_on_close(response.file));
                  }

                  if (settings.length == std::numeric_limits<decltype(settings.length)>::max() ||
                      settings.file_size < settings.length)
                  {
                     settings.length = settings.file_size - settings.offset;
                  }

                  if (settings.offset > settings.file_size)
                  {
                     settings.length = 0;
                  }

                  response.set(http::field::content_length, fmt::to_string(settings.length));

                  if (!settings.write_body || settings.length == 0)
                  {
                     co_return (co_await http::async_write(socket, response, ioe::as_tuple(io::use_awaitable)));
                  }

                  auto [ec, bytes] = co_await http::async_write(socket, response, ioe::as_tuple(io::use_awaitable));
                  if (!ec)
                  {
                     auto [body_ec, body_bytes] =
                        co_await async_file_io::send_file(response.file, socket, settings.offset, settings.length);

                     co_return tuple<error_code, size_t>{std::move(body_ec), body_bytes + bytes};
                  }
                  else
                  {
                     co_return tuple<error_code, size_t>{std::move(ec), bytes};
                  }
               },
               [&](file_byte_ranges_response &response) noexcept -> io::awaitable<tuple<error_code, size_t>>
               {
                  apply_keep_alive(response);

                  auto &settings = response.settings;
                  auto &ranges = settings.ranges;
                  if (ranges.size() == 1)
                  {
                     auto &range = ranges.front();
                     response.set(http::field::content_range,
                                  fmt::format("bytes {}-{}/{}", range.offset_begin,
                                              range.offset_end.value_or(settings.file_size - 1), settings.file_size));

                     const auto length = range.get_length(settings.file_size);
                     response.set(http::field::content_length, fmt::to_string(length));

                     auto [ec, bytes] =
                        co_await http::async_write(socket, response, ioe::as_tuple(io::use_awaitable));
                     if (!ec && settings.write_body)
                     {
                        auto [body_ec, body_bytes] =
                           co_await async_file_io::send_file(response.file, socket, range.offset_begin, length);
                        co_return tuple<error_code, size_t>{std::move(body_ec), bytes + body_bytes};
                     }
                     co_return tuple<error_code, size_t>{ec, bytes};
                  }
                  else
                  {
                     string content_type = static_cast<string>(response[http::field::content_type]);
                     response.set(http::field::content_type,
                                  fmt::format("multipart/byteranges; boundary={}", settings.boundary));

                     const auto boundaries = prepare_byte_range_boundaries(settings.ranges, settings.file_size,
                                                                           settings.boundary, content_type);

                     const auto total_length = get_total_response_body_size(ranges, boundaries, settings.file_size);
                     response.set(http::field::content_length, fmt::to_string(total_length));

                     std::uint64_t total_written = 0;
                     auto [ec, bytes] =
                        co_await http::async_write(socket, response, ioe::as_tuple(io::use_awaitable));
                     total_written += bytes;
                     if (!ec)
                     {
                        for (std::size_t i = 0, len = ranges.size(); i != len; ++i)
                        {
                           auto &prefix = boundaries[i];
                           auto &range = ranges[i];

                           auto [prefix_ec, prefix_bytes] = co_await io::async_write(
                              socket, io::buffer(prefix.c_str(), prefix.size()), ioe::as_tuple(io::use_awaitable));
                           total_written += prefix_bytes;
                           if (prefix_ec)
                           {
                              co_return tuple<error_code, size_t>{std::move(prefix_ec), total_written};
                           }
                           else
                           {
                              const auto length = range.get_length(settings.file_size);
                              auto [body_ec, body_bytes] = co_await async_file_io::send_file(
                                 response.file, socket, range.offset_begin, length);
                              total_written += body_bytes;
                              if (body_ec)
                              {
                                 co_return tuple<error_code, size_t>{std::move(body_ec), total_written};
                              }
                           }
                        }
                        auto &suffix = boundaries.back();
                        auto [suffix_ec, suffix_bytes] = co_await io::async_write(
                           socket, io::buffer(suffix.c_str(), suffix.size()), ioe::as_tuple(io::use_awaitable));
                        total_written += suffix_bytes;
                        co_return tuple<error_code, size_t>{std::move(suffix_ec), total_written};
                     }
                     else
                     {
                        co_return tuple<error_code, size_t>{std::move(ec), bytes};
                     }
                  }
               },
               [&](auto &response_message) noexcept -> io::awaitable<tuple<error_code, size_t>>
               {
                  apply_keep_alive(response_message);
                  return http::async_write(socket, response_message, ioe::as_tuple(io::use_awaitable));
               }},
            message));
      }

      void apply_keep_alive(auto &response) const
      {
         response.keep_alive(keep_alive.has_value() && keep_alive.value().count() > 0);
         if (keep_alive.has_value() && keep_alive.value() > std::chrono::seconds{10})
         {
            // We substract here 1s to keep up with the spec -
            // https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Keep-Alive
            const auto keep_alive_to_send =
               std::chrono::duration_cast<std::chrono::seconds>(keep_alive.value() - std::chrono::seconds{1});
            response.set(http::field::keep_alive, fmt::format("timeout={}", keep_alive_to_send.count()));
         }
      }

      /// this method allows to update all properties of underlying http::response<?>
      template <class Fun>
      auto visit_update(Fun &&updater) -> decltype(updater(const_cast<http::response<http::empty_body> &>(test_obj)))
      {
         using result_type = decltype(updater(const_cast<http::response<http::empty_body> &>(test_obj)));
         if constexpr (std::is_void_v<result_type>)
         {
            std::visit(overload{[](with_no_response &) -> result_type
                                {
                                },
                                std::forward<Fun>(updater)}, message);
         }
         else
         {
            static_assert(std::is_default_constructible_v<result_type>);

            return std::visit(
               overload{[](with_no_response &) -> result_type
                        {
                           return result_type{};
                        },
                        std::forward<Fun>(updater)},
               message);
         }
      }

      /// this method allows to read all properties from underlying http::response<?>
      template <class Fun>
      auto visit_read(Fun &&visitor) const -> decltype(visitor(test_obj))
      {
         using result_type = decltype(visitor(test_obj));
         if constexpr (std::is_void_v<result_type>)
         {
            std::visit(overload{[](with_no_response const &) -> result_type
                                {
                                },
                                std::forward<Fun>(visitor)}, message);
         }
         else
         {
            static_assert(std::is_default_constructible_v<result_type>);
            return std::visit(overload{[](with_no_response const &) -> result_type
                                       {
                                          return result_type{};
                                       },
                                       std::forward<Fun>(visitor)},
                              message);
         }
      }

      auto try_get_string_body() const noexcept -> optional<string_view>
      {
         return visit_read(overload{[](http::response<http::string_body> const &msg) -> optional<string_view>
                                    {
                                       return {msg.body()};
                                    },
                                    [](auto const &) -> optional<string_view>
                                    {
                                       return {};
                                    }});
      }

      template <class response_body>
      auto try_get_message() const noexcept -> http::response<response_body> const *
      {
         return std::visit(
            overload{[](http::response<response_body> const &msg) -> http::response<response_body> const *{
                        return &msg;
                     },
                     [](auto const &) -> http::response<response_body> const *{
                        return nullptr;
                     }},
            message);
      }

      [[nodiscard]] auto get_status() const -> http::status
      {
         auto status = visit_read([](auto &msg) -> optional<http::status>
         {
            return {msg.result()};
         });
         return status.value_or(http::status::not_found);
      }

      [[nodiscard]] inline auto get_headers() const noexcept -> http::fields const *
      {
         return visit_read([](auto const &msg) -> http::fields const *{
            return &msg;
         });
      }

      [[nodiscard]] inline auto get_mutable_headers() noexcept -> http::fields *
      {
         return std::visit(overload{[](with_no_response &) -> http::fields *{
                                       return nullptr;
                                    },
                                    [&](auto &msg) -> http::fields *{
                                       return &msg;
                                    }},
                           message);
      }

      inline auto try_apply_headers(http::fields const &other) noexcept -> bool
      {
         return std::visit(overload{[](with_no_response &) -> bool
                                    {
                                       return false;
                                    },
                                    [&](auto &msg) -> bool
                                    {
                                       copy_headers_to_message(other, msg);
                                       return true;
                                    }},
                           message);
      }

      inline auto try_set_header(http::field header, string_view value) noexcept -> bool
      {
         return std::visit(overload{[](with_no_response &) -> bool
                                    {
                                       return false;
                                    },
                                    [&](auto &msg) -> bool
                                    {
                                       msg.set(header, boost::beast::string_view{value.data(), value.size()});
                                       return true;
                                    }},
                           message);
      }

      inline auto try_set_header(string_view header, string_view value) noexcept -> bool
      {
         return std::visit(overload{[](with_no_response &) -> bool
                                    {
                                       return false;
                                    },
                                    [&](auto &msg) -> bool
                                    {
                                       msg.set(boost::beast::string_view{header.data(), header.size()},
                                               boost::beast::string_view{value.data(), value.size()});
                                       return true;
                                    }},
                           message);
      }

      template <class Body = http::empty_body>
      static auto make_response_message(http::status status) -> http::response<Body>
      {
         auto message = http::response<Body>{};
         message.result(status);

         return message;
      }

      inline static auto return_redirect(string url) -> response
      {
         auto message = make_response_message(http::status::see_other);
         message.set(http::field::location, url);

         return {message};
      }

      inline static auto return_string(http::status status, string body, string content_type = "text/html",
                                       optional<http::fields> custom_headers = {}, bool return_body = true) -> response
      {
         if (return_body)
         {
            auto message = make_response_message<http::string_body>(status);
            if (custom_headers.has_value())
            {
               copy_headers_to_message(custom_headers.value(), message);
            }

            message.set(http::field::content_type, content_type);
            message.set(http::field::content_length, fmt::to_string(body.size()));
            message.body() = std::move(body);
            return {message};
         }
         else
         {
            auto message = make_response_message<http::empty_body>(status);
            if (custom_headers.has_value())
            {
               copy_headers_to_message(custom_headers.value(), message);
            }
            message.set(http::field::content_type, content_type);
            if (status != http::status::not_modified)
            {
               message.set(http::field::content_length, fmt::to_string(body.size()));
            }
            return {message};
         }
      }
   };

   using await_response = io::awaitable<response>;

} // namespace spider2