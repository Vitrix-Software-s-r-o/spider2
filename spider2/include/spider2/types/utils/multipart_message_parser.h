#pragma once
#include "../platform.h"
#include "spider2/types/structs/request_error_code.h"

#include <fmt/base.h>
#include <utility>

namespace spider2
{

   template <class T>
   concept multipart_event_handler = requires(T t) {
      t.on_part_begin(std::declval<http::fields>(), std::declval<error_code &>());
      t.on_part_data(std::declval<std::span<const std::byte>>(), std::declval<error_code &>());
      t.on_part_end(std::declval<error_code &>());
   };

   template <multipart_event_handler EventHandlerT>
   class multipart_message_parser
   {
    public:
      explicit multipart_message_parser(std::string_view boundary, EventHandlerT &handler)
          : boundary_(boundary), handler_(handler)
      {
         buffer_.reserve(8096);
         temp_buffer_.reserve(8096);
         preamble_parser_.header_limit(8096);
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

      void on_finish()
      {
         if (!buffer_.empty())
         {
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

      http::request_parser<http::empty_body> preamble_parser_;
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
            switch (state_)
            {
            case parser_state::preamble:
               on_preamble(it, end, ec);
               buffer_to_front(it, end);
               break;
            case parser_state::part_begin:
               on_part_begin(it, end);
               break;
            case parser_state::part_data:
               on_part_data(it, end);
               break;
            }
         }
      }

      auto buffer_to_front(iterator_type it, iterator_type end) noexcept
      {
         std::copy(it, end, std::back_inserter(temp_buffer_));
         std::swap(buffer_, temp_buffer_);
         temp_buffer_.clear();
      }

      auto on_preamble(iterator_type &it, const iterator_type end, error_code &ec)
      {
         auto consumed = preamble_parser_.put(std::span{reinterpret_cast<const char *>(it.base()),
                                                        static_cast<long unsigned int>(std::distance(it, end))},
                                              ec);
         it += gsl::narrow_cast<int>(consumed);
         if (ec && static_cast<http::error>(ec.value()) != http::error::need_more)
         {
            handler_.on_part_begin({}, ec);
         }
         else if (!ec)
         {
            handler_.on_part_begin(preamble_parser_.release(), ec);
         }
      }

      /// Called when a possible boundary is found.
      /// --boundary--\r\n
      /// ^
      /// @param it - first character of the possible boundary
      /// @param end - end of the buffer
      /// @return
      auto on_possible_boundary(iterator_type it, const iterator_type &end, error_code &ec) -> bool
      {
         if (std::distance(it, end) < boundary_.size() + 6 /* --boundary(--)\r\n*/)
         {
            return true;
         }

         if (if_value_advance<>(it, "--") && if_boundary_advance(it))
         {
            // I found a boundary
            if (if_value_advance<>(it, "--"))
            {
               // I found the end of the message
               state_ = parser_state::end;
               return false;
            }
            else if (if_value_advance<>(it, "\r\n"))
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

      template <size_t N>
      static auto if_value_advance(iterator_type &it, char value[N]) -> bool
      {
         if (std::equal(it, it + (N - 1), value))
         {
            it += (N - 1);
            return true;
         }
         return false;
      }

      auto if_boundary_advance(iterator_type &it) const -> bool
      {
         if (std::equal(it, it + boundary_.size(), boundary_.begin()))
         {
            it += boundary_.size();
            return true;
         }
         return false;
      }
   };
} // namespace spider2