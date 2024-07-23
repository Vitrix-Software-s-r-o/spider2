//
// Created by jhrub on 10.12.2022.
//

#pragma once

// it is super important to include the config.h first
#include "support/config.h"

#include "result/formatter.h"
#include "spider2/routing/api_endpoint/format_api_response.h"
#include <spider2/types.h>

namespace spider2::api
{
   template <class T, class FormatterTag = default_formatter_tag>
   struct result
   {
      /**
       * @brief The type of the value that is returned by the API.
       */
      using value_type = T;

      /**
       * @brief The tag that is used to format the result value.
       * It is used as additional level of indirection for CPO's (format_result_value and
       * get_result_value_content_type).
       */
      using formatter_tag = default_formatter_tag;
      using self_t = result<value_type, formatter_tag>;

      value_type value;
      status result_status;
      optional<http::fields> response_headers;

      result(value_type &&val, status stat = status::ok) : value(std::forward<value_type>(val)), result_status(stat) {}
      result(value_type const &val, status stat = status::ok) : value(val), result_status(stat) {}

      friend auto tag_invoke(tag_t<format_api_response>, request &req, self_t &&res) -> response
      {
         const bool should_format_response =
             res.result_status != http::status::not_modified && req.get_endpoint().verb != http::verb::head;

         return response::return_string(
             res.result_status, should_format_response ? format_result_value(formatter_tag{}, req, res) : "",
             get_result_value_content_type(formatter_tag{}, req, res), res.response_headers, should_format_response);
      }
   };

   template <class ValueT>
   struct is_api_result<result<ValueT>>
   {
      constexpr static std::true_type value{};
   };

} // namespace spider2::api