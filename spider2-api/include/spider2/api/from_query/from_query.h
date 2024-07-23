#pragma once
#include "parser.h"
#include <spider2/routing/api_endpoint/parse_api_request.h>

namespace spider2::api
{
   template <class T>
   struct from_query
   {
      T value = {};

      friend auto tag_invoke(tag_t<parse_api_request>, request &req, from_query<T> &v) -> error_code
      {
         return parse_query(req.get_query_string(), v.value);
      }
   };

} // namespace spider2::api