//
// Created by jhrub on 3/9/2024.
//

#pragma once

#include "parser.h"
#include <spider2/routing/api_endpoint/parse_api_request.h>
namespace spider2::api
{
   template <class T>
   struct from_body
   {
      T value = {};

      friend auto tag_invoke(tag_t<parse_api_request>, request &req, from_body<T> &v) -> error_code
      {
         if (auto string_body_message = req.try_get_message<http::string_body>(); string_body_message != nullptr)
         {
            if (auto it = string_body_message->find(http::field::content_type); it != string_body_message->end())
            {
               auto &content_type = it->value();
               return parse_body(string_view{content_type.data(), content_type.size()}, string_body_message->body(), v.value);
            }
         }
         return make_error_code(request_error_code::api_input_data_error);
      }
   };
} // namespace spider2::api