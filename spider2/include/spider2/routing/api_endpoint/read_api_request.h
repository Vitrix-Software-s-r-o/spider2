//
// Created by jan on 5.3.24.
//

#pragma once

#include "../../types/structs.h"
#include "../../types/utils/tag_invoke.h"

namespace spider2
{
   /**
    * @brief Tag type for api_request customization point
    * Custom types must define a tag_invoke overload for this tag type
    *
    */
   inline constexpr struct read_api_request_fn
   {
      template <class... Args>
      auto operator()(request &req, Args &...args) const -> io::awaitable<error_code>
      {
         return tag_invoke(*this, req, args...);
      }

      template <class... Args>
      friend auto tag_invoke(read_api_request_fn, request &req, Args &...) -> io::awaitable<error_code>
      {
         auto result = error_code{};
         if (req.has_request_body())
         {
            result = co_await req.read_body<http::string_body>();
         }

         co_return result;
      }

   } read_api_request{};

} // namespace spider2