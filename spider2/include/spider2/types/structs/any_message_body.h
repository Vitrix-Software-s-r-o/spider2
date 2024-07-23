//
// Created by jhrub on 3/27/2024.
//

#pragma once
#include "../platform.h"

namespace spider2
{
   class any_message_reader
   {
   public:
      virtual ~any_message_reader() noexcept(false) = default;

      virtual void init(http::fields const & fields,  boost::optional<std::uint64_t> const &length, beast::error_code &ec) = 0;
      virtual std::size_t put(io::const_buffer const &buffers, beast::error_code &ec) = 0;
      virtual void finish(beast::error_code &ec) = 0;
   };

   using any_message_reader_shared_ptr = std::shared_ptr<any_message_reader>;

    struct any_message_body
    {
        using value_type = any_message_reader_shared_ptr;

        class reader
        {
            beast::http::header<isRequest, Fields> &message_header_;
            value_type &body_;
            public:
                template <bool isRequest, class Fields>
                explicit reader(beast::http::header<isRequest, Fields> & header, value_type &body) :
                  message_header_(header),body_(body)
                {
                }

         void init(boost::optional<std::uint64_t> const &length, beast::error_code &ec);

           std::size_t put(io::const_buffer const &buffers, beast::error_code &ec);

           void finish(error_code &ec);
        };


        }


    };
} // namespace spider2