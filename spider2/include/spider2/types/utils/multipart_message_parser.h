#pragma once
#include "../platform.h"
#include "spider2/types/structs/request_error_code.h"

#include <fmt/base.h>
#include <iostream>
#include <ranges>
#include <utility>

namespace spider2
{

   template <class T>
   concept multipart_event_handler = requires(T t) {
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

      /// An empty boundary cannot delimit anything: every token check against it
      /// trivially succeeds and the parser never makes forward progress. The
      /// boundary is attacker-controlled (Content-Type header), so reject it.
      [[nodiscard]] auto has_valid_boundary() const noexcept -> bool { return !boundary_.empty(); }

      void on_data(std::span<const std::byte> data, error_code &ec)
      {
         if (!has_valid_boundary())
         {
            ec = make_error_code(request_error_code::body_read_error);
            return;
         }

         for (std::size_t consumed = 0; consumed < data.size() && !ec;)
         {
            // Bytes to copy this iteration: bounded by free buffer space AND by the
            // bytes still unconsumed in `data`. Using data.size() here (instead of the
            // remaining data.size() - consumed) over-reads past the span once the buffer
            // fills mid-span and the loop comes round again with consumed > 0.
            const auto remaining = std::min(buffer_.capacity() - buffer_.size(), data.size() - consumed);
            if (remaining == 0)
            {
               ec = make_error_code(request_error_code::body_read_error);
               break;
            }

            auto data_it = data.begin();
            std::advance(data_it, consumed);

            if (data_it == data.end())
            {
               break;
            }

            std::copy_n(data_it, remaining, std::back_inserter(buffer_));

            if (buffer_.size() == buffer_.capacity())
            {
               on_process_buffer(ec);
            }
            consumed += remaining;
         }
      }

      void on_finish(error_code &ec)
      {
         if (!has_valid_boundary())
         {
            ec = make_error_code(request_error_code::body_read_error);
            return;
         }

         if (!buffer_.empty() && state_ != parser_state::end)
         {
            on_process_buffer(ec);

            if (state_ != parser_state::end && !ec)
            {
               ec = make_error_code(request_error_code::body_read_error);
            }
         }
      }

    private:
      enum class parser_state
      {
         preamble,
         part_begin,
         part_data,
         end,
         at_boundary
      };

      using iterator_type = std::vector<std::byte>::const_iterator;

      parser_state state_ = parser_state::preamble;

      std::string_view boundary_;
      std::vector<std::byte> buffer_;
      std::vector<std::byte> temp_buffer_;

      EventHandlerT &handler_;

      void on_process_buffer(error_code &ec)
      {
         auto it = buffer_.cbegin();
         const auto end = buffer_.cend();

         for (; it != end;)
         {
            const bool need_more_data = [this](auto &it, auto const &end, error_code &ec) -> bool
            {
               switch (state_)
               {
               case parser_state::preamble:
                  return on_preamble(it, end, ec);
               case parser_state::part_begin:
                  return on_part_begin(it, end, ec);
               case parser_state::part_data:
                  return on_part_data(it, end, ec);
               case parser_state::at_boundary:
                  return on_at_boundary(it, end, ec);
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
               if (buffer_.size() == buffer_.capacity())
               {
                  ec = make_error_code(request_error_code::body_read_error);
                  handler_.on_finish(ec);
               }
               break;
            }
         }
      }

      auto on_part_begin(iterator_type &it, iterator_type end, error_code &ec) -> bool
      {
         // RFC 2046: "NO header fields are actually required in body parts."
         // Check if we have an immediate CRLF (no headers case)
         if (advance_if_equals(it, end, "\r\n"))
         {
            // No headers, create empty headers and transition to part_data
            handler_.on_part_begin(http::fields{}, ec);
            state_ = parser_state::part_data;
            return false;
         }

         // Look for headers terminated by \r\n\r\n
         for (auto search_it = std::find(it, end, std::byte{'\r'}); search_it != end;
              search_it = std::find(search_it + 1, end, std::byte{'\r'}))
         {
            if (advance_if_equals(search_it,end, "\r\n\r\n"))
            {
               auto str = std::string(reinterpret_cast<const char *>(&*it),
                                      gsl::narrow_cast<std::size_t>(std::distance(it, search_it)));

               auto part_header_parser = http::request_parser<http::empty_body>{};
               part_header_parser.header_limit(max_part_header_size);

               constexpr std::string_view fake_request_line = "GET / HTTP/1.1\r\n";
               part_header_parser.put(net::const_buffer{fake_request_line.data(), fake_request_line.size()}, ec);
               part_header_parser.put(
                   net::const_buffer{&*it, gsl::narrow_cast<std::size_t>(std::distance(it, search_it))}, ec);
               part_header_parser.put_eof(ec);

               if (ec)
               {
                  return false;
               }

               handler_.on_part_begin(part_header_parser.release(), ec);
               // I found the beginning of a part
               state_ = parser_state::part_data;

               // all up to end of the header is processed
               it = search_it;
               return false;
            }
         }

         // need more input
         return true;
      }

      auto on_part_data(iterator_type &it, const iterator_type end, error_code &ec) -> bool
      {
         auto search_it = it;

         const auto flush_part = [&]()
         {
            if (it != search_it && !ec)
            {
               handler_.on_part_data(
                   std::span<const std::byte>{&*it, gsl::narrow_cast<std::size_t>(std::distance(it, search_it))}, ec);
               it = search_it;
            }
         };

         const auto find_first_possibility_of_boundary = [&]()
         {
            if (search_it == end)
            {
               return search_it;
            }

            if (*search_it == std::byte{'-'})
            {
               // this is the case where we could immediately have a boundary after part headers
               return search_it;
            }

            return std::find(search_it, end, std::byte{'\r'});
         };

         for (search_it = find_first_possibility_of_boundary(); search_it != end && !ec;
              search_it = std::find(search_it + 1, end, std::byte{'\r'}))
         {
            auto boundary_it = search_it;
            if (this->on_possible_boundary(boundary_it, end, ec))
            {
               // we need more data to determine if this is a boundary
               flush_part();
               return true;
            }

            if (state_ == parser_state::at_boundary)
            {
               // flushing the part data
               // search_it points to the first character of the boundary
               flush_part();

               handler_.on_part_end(ec);

               // advance the iterator to the end of the boundary
               it = boundary_it;
               return false;
            }
         }

         // if we reached the end of the buffer, we need more data
         flush_part();
         return true;
      }

      auto buffer_to_front(iterator_type it, iterator_type end) noexcept
      {
         std::copy(it, end, std::back_inserter(temp_buffer_));
         std::swap(buffer_, temp_buffer_);
         temp_buffer_.clear();
      }

      /// Wait for the preamble to be found --boundary--\r\n
      /// \returns true if more data is needed
      auto on_preamble(iterator_type &it, const iterator_type &end, error_code &ec) -> bool
      {
         if (auto p_it = std::min(std::find(it, end, std::byte{'-'}), std::find(it, end, std::byte{'\r'})); p_it != end)
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
      /// \r\n--boundary--\r\n
      /// ^
      /// @param it - first character of the possible boundary
      /// @param end - end of the buffer
      /// @return true if more data is needed
      auto on_possible_boundary(iterator_type &it, const iterator_type &end, error_code &ec) -> bool
      {
         if (std::distance(it, end) < gsl::narrow_cast<std::ptrdiff_t>(boundary_.size()) + 4 /* --boundary */)
         {
            return true;
         }

         // Probe on a copy: advance_if_equals mutates its iterator as a side effect, so
         // committing `it` directly would leave it advanced (possibly past `end`) when the
         // delimiter only partially matches. Only adopt the probe once the whole token matches.
         if (auto probe = it; advance_if_equals(probe, end, "\r\n--") && if_boundary_advance(probe, end))
         {
            it = probe;
            state_ = parser_state::at_boundary;
            return false;
         }

         if (auto probe = it; advance_if_equals(probe, end, "--") && if_boundary_advance(probe, end))
         {
            it = probe;
            state_ = parser_state::at_boundary;
            return false;
         }

         // Not a boundary here; step over this candidate byte so the scan makes progress.
         if (it != end)
         {
            ++it;
         }
         return false;
      }

      auto on_at_boundary(iterator_type &it, const iterator_type &end, error_code &ec) -> bool
      {
         if (std::distance(it, end) < 2)
         {
            return true;
         }

         // I found a boundary
         if (advance_if_equals(it, end, "--"))
         {
            // I found the end of the message
            state_ = parser_state::end;
            return false;
         }
         else if (advance_if_equals(it, end,"\r\n"))
         {
            // I found the beginning of a part
            state_ = parser_state::part_begin;
            return false;
         }
         else
         {
            // there must be either "--" or "\r\n" after the boundary
            ec = make_error_code(request_error_code::body_read_error);
            return false;
         }
      }

      /// This method advances the iterator by the length of the value if current
      /// iterator points to the exact sequence of characters in the value.
      ///
      /// @param it - iterator to be advanced
      /// @param value - value to be compared
      /// @return true - if the @it contained sequence specified by value by its length and was advanced, false -
      /// otherwise
      static auto advance_if_equals(iterator_type &it, const iterator_type &end, std::string_view value) -> bool
      {
         static_assert(std::is_same_v<iterator_type::value_type, std::byte>);
         const auto len = gsl::narrow_cast<std::ptrdiff_t>(value.size());
         // The whole token must be present; a partial prefix must not match (it would
         // otherwise advance `it` past `end` and corrupt every subsequent iterator op).
         if (std::distance(it, end) < len)
         {
            return false;
         }

         if (std::equal(value.begin(), value.end(), reinterpret_cast<const char *>(&*it)))
         {
            it += len;
            return true;
         }

         return false;
      }

      /// This method advances the iterator if it points to the boundary and checks if it is followed by either "--"
      /// (end of message) or "\r\n" (beginning of part).
      ///
      /// @param it - iterator to be advanced
      /// @return true - if the @it contained sequence specified by value by its length and was advanced, false -
      /// otherwise
      auto if_boundary_advance(iterator_type &it, const iterator_type& end) const -> bool
      {
         if (auto boundary_it = it; advance_if_equals(boundary_it, end, boundary_))
         {
            if (auto after_boundary_it = boundary_it; advance_if_equals(after_boundary_it, end, "--"))
            {
               it = boundary_it;
               return true;
            }
            else if (advance_if_equals(after_boundary_it,end, "\r\n"))
            {
               it = boundary_it;
               return true;
            }
         }

         return false;
      }
   };
} // namespace spider2