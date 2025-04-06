//
// Created by jhrub on 3/27/2024.
//

#pragma once
#include "../platform.h"
#include "request_error_code.h"

namespace spider2
{
   class any_message_reader
   {
    public:
      virtual ~any_message_reader() noexcept(false) = default;

      virtual void init(http::fields const &fields, boost::optional<std::uint64_t> const &length, error_code &ec) = 0;

      virtual std::size_t put(std::size_t allocation_hint_size, std::span<const std::byte>, error_code &ec) = 0;

      virtual void finish(error_code &ec) = 0;
   };

   struct any_message_body
   {
      using value_type = any_message_reader *;

      class reader
      {
         http::fields message_header_;
         value_type &body_;

       public:
         template <bool isRequest, class Fields>
         explicit reader(http::header<isRequest, Fields> &header, value_type &body)
             : message_header_(header), body_(body)
         {
         }

         auto init(boost::optional<std::uint64_t> const &length, error_code &ec)
         {
            if (body_)
               body_->init(message_header_, length, ec);
            else
               ec = make_error_code(request_error_code::body_read_error_no_parser);
         }

         template <class ConstBufferSequence>
         auto put(ConstBufferSequence const &buffers, error_code &ec) -> std::size_t
         {
            if (body_)
            {
               std::size_t total = buffer_bytes(buffers);

               std::size_t result = 0;
               for (auto &&buffer : buffers_range_ref(buffers))
               {
                  result += body_->put(
                      total, std::span<const std::byte>{static_cast<const std::byte *>(buffer.data()), buffer.size()},
                      ec);
                  if (ec)
                  {
                     break;
                  }
               }

               return result;
            }
            else
            {
               ec = make_error_code(request_error_code::body_read_error_no_parser);
            }
            return 0;
         }

         auto finish(error_code &ec)
         {
            if (body_)
               body_->finish(ec);
            else
               ec = make_error_code(request_error_code::body_read_error_no_parser);
         }
      };
   };
}; // namespace spider2
