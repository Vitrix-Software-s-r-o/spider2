#pragma once
#include "../platform.h"
#include "spider2/types/structs/request_error_code.h"

#include <iostream>
#include <ranges>
#include <fmt/base.h>
#include <utility>

namespace spider2
{

   template <class T>
   concept multipart_event_handler = requires(T t)
   {
      t.on_part_begin(std::declval<http::fields>(), std::declval<error_code &>());
      t.on_part_data(std::declval<std::span<const std::byte>>(), std::declval<error_code &>());
      t.on_part_end(std::declval<error_code &>());
      t.on_finish(std::declval<error_code &>());
   };

   template <multipart_event_handler EventHandlerT>
   class multipart_message_parser
   {
   public:
      constexpr static std::size_t max_part_header_size = 8096;

      explicit multipart_message_parser(std::string_view boundary, EventHandlerT &handler)
         : boundary_(boundary), handler_(handler)
      {
         buffer_.reserve(max_part_header_size);
         temp_buffer_.reserve(max_part_header_size);
      }

      void on_data(std::span<const std::byte> data, error_code &ec)
      {
         for (std::size_t consumed = 0; consumed < data.size();)
         {
            const auto remaining = std::min(buffer_.capacity() - buffer_.size(), data.size());
            std::copy_n(data.begin(), remaining, std::back_inserter(buffer_));

            if (buffer_.size() == buffer_.capacity())
            {
               on_process_buffer(ec);
            }
            consumed += remaining;
         }
      }

      void on_finish(error_code &ec)
      {
         if (!buffer_.empty() && state_ != parser_state::end)
         {
            on_process_buffer(ec);
         }
      }

      [[nodiscard]]
      auto get_processed_size() const -> std::size_t
      {
         return processed_;
      }

   private:
      enum class parser_state
      {
         preamble,
         part_begin,
         part_data,
         end
      };

      using iterator_type = std::vector<std::byte>::const_iterator;

      parser_state state_ = parser_state::preamble;

      std::string_view boundary_;
      std::vector<std::byte> buffer_;
      std::vector<std::byte> temp_buffer_;

      std::size_t processed_ = 0;
      EventHandlerT &handler_;

      void on_process_buffer(error_code &ec)
      {
         auto it = buffer_.cbegin();
         const auto end = buffer_.cend();

         for (; it != end;)
         {
            auto current_unprocessed = std::string {reinterpret_cast<const char*>(&*it), gsl::narrow_cast<std::size_t>(std::distance(it, end))};
            const bool need_more_data = [this](auto &it, auto const & end, error_code &ec) -> bool
            {
               switch (state_)
               {
               case parser_state::preamble:
                  return on_preamble(it, end, ec);
               case parser_state::part_begin:
                  return on_part_begin(it, end, ec);
               case parser_state::part_data:
                  return on_part_data(it, end, ec);
               default:
                  ec = make_error_code(request_error_code::body_read_error);
                  return false;
               }
            }(it, end, ec);

            if (ec)
            {
               state_ = parser_state::end;
            }

            if (state_ == parser_state::end)
            {
               handler_.on_finish(ec);
               break;
            }

            if (need_more_data)
            {
               buffer_to_front(it, end);
               break;
            }
         }
      }

      auto on_part_begin(iterator_type &it, iterator_type end, error_code &ec) -> bool
      {
         for (auto search_it = std::find(it, end, std::byte{'\r'});
            search_it != end; search_it = std::find(it + 1, end, std::byte{'\r'}))
         {
            if (advance_if_equals(search_it, "\r\n\r\n"))
            {
               auto str = std::string(reinterpret_cast<const char*>(&*it), gsl::narrow_cast<std::size_t>(std::distance(it, search_it)));

               auto part_header_parser = http::request_parser<http::empty_body>{};
               part_header_parser.header_limit(max_part_header_size);

               constexpr std::string_view fake_request_line = "GET / HTTP/1.1\r\n";
               part_header_parser.put(net::const_buffer{fake_request_line.data(), fake_request_line.size()}, ec);
               part_header_parser.put(
                  net::const_buffer{&*it, gsl::narrow_cast<std::size_t>(std::distance(it, search_it))}, ec);
               part_header_parser.put_eof(ec);


               if (!ec)
               {
                  handler_.on_part_begin(part_header_parser.release(), ec);
                  // I found the beginning of a part
                  state_ = parser_state::part_data;

                  // all up to end of the header is processed
                  it = search_it;
                  return false;
               }
            }
         }

         //need more input
         return true;
      }

      auto on_part_data(iterator_type &it, iterator_type end, error_code &ec) -> bool
      {
         iterator_type search_it = it;

         const auto flush_part = [&]()
         {
            if (it != search_it && !ec)
            {
               handler_.on_part_data(
                  std::span<const std::byte>{&*it, gsl::narrow_cast<std::size_t>(std::distance(it, search_it))}, ec);
               it = search_it;
            }
         };

         bool needs_more_data = false;
         for (search_it = std::find(search_it, end, std::byte{'-'}); search_it != end && !ec && !needs_more_data;
              search_it = std::find(search_it + 1, end, std::byte{'-'}))
         {
            auto boundary_it = search_it;
            needs_more_data = this->on_possible_boundary(boundary_it, end, ec);
            flush_part();

            if (state_ != parser_state::part_data)
            {
               handler_.on_part_end(ec);
               it = boundary_it;
               return false;
            }
         }

         flush_part();
         return false;
      }

      auto buffer_to_front(iterator_type it, iterator_type end) noexcept
      {
         std::copy(it, end, std::back_inserter(temp_buffer_));
         std::swap(buffer_, temp_buffer_);
         temp_buffer_.clear();
      }

      /// Wait for the preamble to be found --boundary--\r\n
      /// \returns true if more data is needed
      auto on_preamble(iterator_type &it, const iterator_type end, error_code &ec) -> bool
      {
         if (auto p_it = std::find(it, end, std::byte{'-'}); p_it != end)
         {
            if (!this->on_possible_boundary(p_it, end, ec))
            {
               it = p_it;
               return false;
            }
         }
         return true;
      }

      /// Called when a possible boundary is found.
      /// --boundary--\r\n
      /// ^
      /// @param it - first character of the possible boundary
      /// @param end - end of the buffer
      /// @return true if more data is needed
      auto on_possible_boundary(iterator_type &it, const iterator_type &end, error_code &ec) -> bool
      {
         if (std::distance(it, end) < boundary_.size() + 6 /* --boundary(--)\r\n*/)
         {
            return true;
         }

         if (advance_if_equals(it, "--") && if_boundary_advance(it))
         {
            // I found a boundary
            if (advance_if_equals(it, "--"))
            {
               // I found the end of the message
               state_ = parser_state::end;
               return false;
            }
            else if (advance_if_equals(it, "\r\n"))
            {
               // I found the beginning of a part
               state_ = parser_state::part_begin;
               return false;
            }
            else
            {
               ec = make_error_code(request_error_code::body_read_error);
            }
         }

         ++it;
         return false;
      }


      auto advance_if_equals(iterator_type &it, std::string_view value) const -> bool
      {
         static_assert(std::is_same_v<iterator_type::value_type, std::byte>);
         if (const auto len = gsl::narrow_cast<int>(value.size());
            std::equal(&*it, (&*it) + len, reinterpret_cast<const std::byte *>(value.data())))
         {
            it += len;
            return true;
         }

         return false;
      }

      auto if_boundary_advance(iterator_type &it) const -> bool
      {
         return advance_if_equals(it, boundary_);
      }
   };
} // namespace spider2