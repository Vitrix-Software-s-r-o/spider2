#pragma once
#ifdef SPIDER2_API_USE_VITRIX_COMMONS

#include <spider2/api/from_body/parser.h>
#include <spider2/api/result/formatter.h>

#include "support_decl.h"
#include <spider2/api/from_query/query_args.h>

#include <serialization/serialization.h>

/**
 *  Include this file if you want to use visit_struct based serialization in your API.
 */
namespace spider2::api
{
   template <class T>
   concept visitable_value = visit_struct::traits::is_visitable<T>::value;

   template <class T>
   concept api_result_with_serializable_value =
       api_result_c<T> && (visitable_value<typename T::value_type> || std::ranges::range<typename T::value_type>);
   ;

   auto tag_invoke(tag_t<format_result_value>, commons_serialization_formatter_tag, request &,
                   api_result_with_serializable_value auto const &resp) -> string
   {
      return spider::serialization::json::stringify_json(resp.value);
   }

   auto tag_invoke(tag_t<parse_query>, string_view query, visitable_value auto &output) -> error_code
   {
      if (!spider::serialization::url_query::parse_url_query(output, query))
      {
         return make_error_code(request_error_code::api_input_data_error);
      }

      return {};
   }

   auto tag_invoke(tag_t<parse_json>, string_view body, visitable_value auto &output) -> error_code
   {
      if (!spider::serialization::json::parse_json(output, body))
      {
         return make_error_code(request_error_code::api_input_data_error);
      }
      return {};
   }
} // namespace spider2::api
#endif