#pragma once
#include <boost/json.hpp>
#include <spider2/api/from_body/parser.h>
#include <spider2/api/result/formatter.h>

#include "support_decl.h"
#include <spider2/api/from_query/query_args.h>

/**
 *  Include this file if you want to use Boost.JSON in your API.
 */
namespace spider2::api
{
   auto tag_invoke(tag_t<format_result_value>, boost_json_formatter_tag, request &, api_result_c auto const &resp)
       -> string
   {
      auto jv = boost::json::value_from(resp.value);
      return boost::json::serialize(jv);
   }

   template <class T>
   auto tag_invoke(tag_t<parse_json>, string_view body, T &output) -> error_code
   {
      auto ec = error_code{};
      auto jv = boost::json::parse(body, ec);
      if (ec)
      {
         return ec;
      }

      output = boost::json::value_to<T>(jv);

      return {};
   }
} // namespace spider2::api