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
   inline constexpr struct parse_api_request_fn
   {
      template <class T>
      auto operator()(request &r, T &v) const -> error_code
      {
         return tag_invoke(*this, r, v);
      }

      template <class T>
      friend auto tag_invoke(parse_api_request_fn, request &, T &) -> error_code
      {
         // please provide your own implementation
         return make_error_code(request_error_code::not_implemented);
      }
   } parse_api_request{};

} // namespace spider2