//
// Created by jhrub on 3/10/2024.
//

#pragma once
#include <spider2/types.h>

namespace spider2::api
{
   inline constexpr struct parse_query_fn
   {
      template <class T>
      auto operator()(string_view query, T &output) const -> error_code
      {
         return tag_invoke(*this, query, output);
      }

      friend auto tag_invoke(parse_query_fn, string_view, auto &) -> error_code
      {
         return make_error_code(request_error_code::not_implemented);
      }
   } parse_query{};
} // namespace spider2::api