//
// Created by jhrub on 11.12.2022.
//

#pragma once

#include "spider2/types/platform.h"
#include "spider2/types/utils/overload.h"

#include "any_message_body.h"
#include "endpoint.h"
#include "request_error_code.h"

namespace spider2
{
   struct app_context_base;

   struct request
   {
      using any_message_type = std::variant<http::request<http::empty_body>, http::request<http::string_body>,
                                            http::request<any_message_body>>;

      inline request(tcp::socket &s, flat_buffer &b, app_context_base &ctx);

      template <class BodyT>
      inline request(http::request<BodyT> direct_request, app_context_base &ctx);

      // TODO make this a solid type and switch the request body by variant
      // TODO we need only variant<empty_body,string_body,multipart_request, custom_body with provided shared_ptr>
      virtual ~request() noexcept(false) = default;

      [[nodiscard]] inline auto get_executor() noexcept -> tcp::socket::executor_type;

      [[nodiscard]] inline auto has_keep_alive() const noexcept -> bool;

      [[nodiscard]] inline auto get_headers() const noexcept -> http::fields const &;

      [[nodiscard]] inline auto get_endpoint() const noexcept -> endpoint_base;

      [[nodiscard]] inline auto get_processing_endpoint() const noexcept -> endpoint_base;

      [[nodiscard]] inline auto get_raw_path() const noexcept -> string_view;

      [[nodiscard]] inline auto get_query_string() const noexcept -> string_view;

      [[nodiscard]] inline auto get_raw_path_include_query() const noexcept -> string_view;

      [[nodiscard]] inline auto get_client_ip_address() const noexcept -> const ip_address &;

      [[nodiscard]] inline auto get_request_id() const noexcept -> std::int64_t;

      [[nodiscard]] inline auto get_request_server_tcp_endpoint() const noexcept -> tcp::endpoint;

      template <class BodyT>
      [[nodiscard]] inline auto try_get_message() const noexcept -> const http::request<BodyT> *;

      template <class BodyT>
      [[nodiscard]] inline auto try_get_message() noexcept -> http::request<BodyT> *;

      [[nodiscard]] inline auto steal_socket() noexcept -> tcp::socket;

      [[nodiscard]] inline auto is_socket_stolen() const noexcept -> bool
      {
         return socket_ == nullptr;
      }

      [[nodiscard]] inline auto read_header() noexcept -> io::awaitable<error_code>;

      template <class BodyT>
      [[nodiscard]] inline auto read_body(optional<std::uint64_t> body_limit = {}) noexcept
         -> io::awaitable<error_code>;

      [[nodiscard]] inline auto read_body(any_message_reader &reader, optional<std::uint64_t> body_limit = {}) noexcept
         -> io::awaitable<error_code>;

      [[nodiscard]] inline auto has_request_body() const noexcept -> bool;

      [[nodiscard]] inline auto push_path_processing_offset(std::uint32_t offset) noexcept
      {
         struct offset_stack_handle
         {
            request &req;
            std::uint32_t offset;

            ~offset_stack_handle()
            {
               req.path_processing_offset_ -= offset;
            }
         };
         path_processing_offset_ += offset;
         return offset_stack_handle{*this, offset};
      }

      inline auto get_app_context() -> app_context_base &
      {
         return ctx_;
      }

      inline auto get_app_context() const -> app_context_base const &
      {
         return ctx_;
      }

   protected:
      tcp::socket *socket_ = nullptr;
      flat_buffer *buffer_ = nullptr;
      app_context_base &ctx_;

      http::request_parser<http::empty_body> parser_;
      any_message_type message_;

      bool keep_alive_ = false;

      string raw_path_include_query_;
      http::verb verb_;
      ip_address client_ip_;
      std::uint32_t query_offset_ = 0;
      std::uint32_t path_processing_offset_ = 0;
      std::int64_t request_id_ = 0;
      bool read_body_called_ = false;

      template <class BodyT>
      void fill(http::request<BodyT> &msg)
      {
         auto path_and_query = [&]
         {
            auto target = msg.target();
            return std::string{target.data(), target.size()};
         }();

         auto query_offset = [&]
         {
            size_t query_idx = path_and_query.find('?');
            if (query_idx == string_view::npos)
            {
               return -1;
            }
            else
            {
               return static_cast<std::int32_t>(query_idx);
            }
         }();

         raw_path_include_query_ = std::move(path_and_query);
         query_offset_ = query_offset;
         keep_alive_ = msg.keep_alive();
         verb_ = msg.method();
      }

      void disconnect_on_error()
      {
         boost::system::error_code ec;
         socket_->shutdown(boost::asio::socket_base::shutdown_both, ec);
         socket_->close(ec);
      }

      [[nodiscard]]
      inline auto on_read_body_init() const -> error_code;

      template <class BodyT>
      auto on_set_request_limit(http::request_parser<BodyT> &body_parser, std::optional<std::size_t> const &limit) const
         -> void;

      template <class BodyT>
      [[nodiscard]]
      auto read_body_from_socket(http::request_parser<BodyT> &parser) -> io::awaitable<error_code>;
   };

   request::request(tcp::socket &s, flat_buffer &b, app_context_base &ctx)
      : socket_(&s), buffer_(&b), ctx_(ctx)
   {
      auto r_ec = error_code{};
      const auto remote_endpoint = socket_->remote_endpoint(r_ec);
      if (!r_ec)
      {
         client_ip_ = remote_endpoint.address();
      }
   }

   template <class BodyT>
   request::request(http::request<BodyT> direct_request, app_context_base &ctx)
      : socket_(nullptr), ctx_(ctx)
   {
      client_ip_ = io::ip::address_v4::loopback();
      fill(direct_request);
      message_ = std::move(direct_request);
      auto exhaust = string_view{"GET / HTTP/1.1\r\nHost: dummy.org\r\n\r\n"};
      auto ec = boost::beast::error_code{};
      parser_.put(net::const_buffer{exhaust.data(), exhaust.size()}, ec);

      read_body_called_ = true;
   }

   [[nodiscard]] inline auto request::get_executor() noexcept -> tcp::socket::executor_type
   {
      if (socket_ == nullptr) std::terminate();

      return socket_->get_executor();
   }

   inline auto request::has_keep_alive() const noexcept -> bool
   {
      return keep_alive_;
   }

   inline auto request::get_headers() const noexcept -> http::fields const &
   {
      if (read_body_called_ || parser_.is_done())
      {
         return std::visit([](auto &message) -> http::fields const &
         {
            return message;
         }, this->message_);
      }
      else
      {
         return this->parser_.get();
      }
   }

   inline auto request::get_endpoint() const noexcept -> endpoint_base
   {
      auto path = string_view{raw_path_include_query_};
      if (query_offset_ > 0)
      {
         path = path.substr(0, query_offset_);
      }

      return endpoint_base{verb_, path};
   }

   inline auto request::get_processing_endpoint() const noexcept -> endpoint_base
   {
      auto full_path = get_endpoint();
      full_path.path = full_path.path.substr(path_processing_offset_);
      return full_path;
   }

   inline auto request::get_raw_path() const noexcept -> string_view
   {
      if (query_offset_ == std::numeric_limits<std::uint32_t>::max())
      {
         return string_view{raw_path_include_query_};
      }
      else
      {
         return string_view{raw_path_include_query_}.substr(0, query_offset_ - 1);
      }
   }

   inline auto request::get_query_string() const noexcept -> string_view
   {
      if (query_offset_ == std::numeric_limits<std::uint32_t>::max())
      {
         return {};
      }
      else
      {
         return string_view{raw_path_include_query_}.substr(query_offset_ + 1);
      }
   }

   inline auto request::get_raw_path_include_query() const noexcept -> string_view
   {
      return string_view{raw_path_include_query_};
   }

   inline auto request::get_client_ip_address() const noexcept -> const ip_address &
   {
      return client_ip_;
   }

   inline auto request::get_request_id() const noexcept -> std::int64_t
   {
      return request_id_;
   }

   inline auto request::get_request_server_tcp_endpoint() const noexcept -> tcp::endpoint
   {
      return socket_->local_endpoint();
   }

   inline auto request::steal_socket() noexcept -> tcp::socket
   {
      if (socket_ == nullptr)
      {
         std::terminate();
      }
      return std::move(*std::exchange(socket_, nullptr));
   }

   template <class BodyT>
   [[nodiscard]] auto request::try_get_message() const noexcept -> const http::request<BodyT> *
   {
      return std::visit(overload{[](http::request<BodyT> const &msg) -> decltype(&msg)
                                 {
                                    return &msg;
                                 },
                                 [](auto &) -> http::request<BodyT> const *
                                 {
                                    return nullptr;
                                 }},
                        message_);
   }

   template <class BodyT>
   [[nodiscard]] auto request::try_get_message() noexcept -> http::request<BodyT> *
   {
      return std::visit(overload{[](http::request<BodyT> &msg) -> decltype(&msg)
                                 {
                                    return &msg;
                                 },
                                 [](auto &) -> http::request<BodyT> *
                                 {
                                    return nullptr;
                                 }},
                        message_);
   }

   inline auto request::read_header() noexcept -> io::awaitable<error_code>
   {
      if (socket_ == nullptr)
      {
         co_return make_error_code(request_error_code::socket_stolen);
      }

      if (parser_.is_header_done())
      {
         co_return error_code{};
      }

      static std::atomic<std::int64_t> request_ids = 0;
      request_id_ = request_ids++;
      message_ = {};
      path_processing_offset_ = 0;
      query_offset_ = std::numeric_limits<std::uint32_t>::max();
      raw_path_include_query_.clear();

      auto [ec, bytes] =
         co_await http::async_read_header(*socket_, *buffer_, parser_, ioe::as_tuple(io::use_awaitable));
      if (!ec)
      {
         fill(parser_.get());

         if (parser_.is_done())
         {
            message_ = parser_.release();
         }
      }
      else
      {
         disconnect_on_error();
      }

      co_return ec;
   }

   template <class BodyT>
   [[nodiscard]] inline auto request::read_body(optional<std::uint64_t> body_limit) noexcept
      -> io::awaitable<error_code>
   {
      static_assert(!std::is_same_v<BodyT, any_message_body>, "Use a later overload for any_message_body");
      if (read_body_called_)
      {
         co_return error_code{};
      }

      if (auto ec = this->on_read_body_init(); ec)
      {
         co_return ec;
      }
      read_body_called_ = true;

      auto body_parser = http::request_parser<BodyT>{std::move(parser_)};

      on_set_request_limit(body_parser, body_limit);

      co_return (co_await read_body_from_socket(body_parser));
   }

   inline auto request::read_body(any_message_reader &reader, optional<std::uint64_t> body_limit) noexcept
      -> io::awaitable<error_code>
   {
      if (auto ec = this->on_read_body_init(); ec)
      {
         co_return ec;
      }

      read_body_called_ = true;

      auto body_parser = http::request_parser<any_message_body>{std::move(parser_)};
      on_set_request_limit(body_parser, body_limit);

      body_parser.get().body() = &reader;

      co_return (co_await read_body_from_socket(body_parser));
   }

   auto request::on_read_body_init() const -> error_code
   {
      auto precondition_ec = [&]()
      {
         auto result = optional<error_code>{};
         if (read_body_called_)
         {
            result.emplace(error_code{}); // already read
         }
         else if (socket_ == nullptr)
         {
            result = make_error_code(request_error_code::socket_stolen);
         }
         else if (!parser_.is_header_done())
         {
            result = make_error_code(request_error_code::body_read_error);
         }

         return result;
      }();

      if (precondition_ec.has_value())
      {
         return precondition_ec.value();
      }

      return {};
   }

   template <class BodyT>
   auto request::on_set_request_limit(http::request_parser<BodyT> &body_parser,
                                      std::optional<std::size_t> const &body_limit) const -> void
   {
      if constexpr (std::is_same_v<BodyT, http::string_body>)
      {
         body_parser.body_limit(body_limit.value_or(10000000));
      }
      else
      {
         body_parser.body_limit(body_limit.value_or(static_cast<std::uint64_t>(-1)));
      }
   }

   template <class BodyT>
   auto request::read_body_from_socket(http::request_parser<BodyT> &body_parser) -> io::awaitable<error_code>
   {
      if (socket_ == nullptr)
      {
         co_return make_error_code(request_error_code::socket_stolen);
      }

      auto [ec, bytes] = co_await http::async_read(*socket_, *buffer_, body_parser, ioe::as_tuple(io::use_awaitable));
      if (!ec)
      {
         message_ = body_parser.release();
      }
      else
      {
         disconnect_on_error();
      }
      co_return ec;
   }

   [[nodiscard]] inline auto request::has_request_body() const noexcept -> bool
   {
      switch (verb_)
      {
      case verb::post:
      case verb::put:
         return true;
      default:
         return false;
      }
   }
} // namespace spider2