#pragma once

#include "spider2/api/from_query/parser.h"

#include <spider2/types.h>

namespace spider2::api
{
   inline constexpr struct parse_json_fn
   {
      template <class T>
      auto operator()(string_view body, T &output) const -> error_code
      {
         return tag_invoke(*this, body, output);
      }
   } parse_json{};

   inline constexpr struct parse_body_fn
   {
      template <class T>
      auto operator()(string_view content_type, string_view body, T &output) const -> error_code
      {
         return tag_invoke(*this, content_type, body, output);
      }

      template <class T>
      friend auto tag_invoke(parse_body_fn, string_view content_type, string_view body, T &output) -> error_code
      {
         if (content_type == "application/x-www-form-urlencoded")
         {
            return parse_query(body, output);
         }
         else if (content_type == "application/json")
         {
            return parse_json(body, output);
         }
         return make_error_code(request_error_code::not_implemented);
      }
   } parse_body{};
} // namespace spider2::api