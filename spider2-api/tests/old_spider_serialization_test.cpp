
#ifdef SPIDER2_API_USE_VITRIX_COMMONS
#include <catch.hpp>
#include <spider2/api/support/commons_serialization/use_commons_serialization.h>
#include <visit_struct/visit_struct.hpp>

#include <spider2/api/from_body/from_body.h>
#include <spider2/api/result.h>
#include <spider2/testing.h>

using namespace spider2;

struct custom_type
{
   string value;
};
VISITABLE_STRUCT(custom_type, value);

namespace
{
   auto dummy_request = []()
   {
      static app_context_base app_context;
      return request{http::request<http::empty_body>{verb::get, "/", 11}, app_context};
   }();

   auto make_str_req(const string &path_with_query, const string &body, const string &content_type)
   {
      static app_context_base app_context;
      auto message = http::request<http::string_body>{verb::post, path_with_query, 11};
      message.body() = body;
      message.set(http::field::content_type, content_type);
      message.content_length(body.size());

      return request{std::move(message), app_context};
   }
} // namespace

TEST_CASE("test custom_response with commons serialization", "[result]")
{
   auto result_message = api::result{custom_type{.value = "test string"}};
   static_assert(api::is_api_result_v<decltype(result_message)>);

   auto resp = format_api_response(dummy_request, std::move(result_message));
   auto *fields = resp.get_headers();
   REQUIRE(fields != nullptr);
   REQUIRE(fields->at(http::field::content_type) == "application/json");
   REQUIRE(resp.get_status() == http::status::ok);
   REQUIRE(resp.try_get_string_body().value_or("") == "{\"value\":\"test string\"}");
}

TEST_CASE("parse request body with commons serialization", "[from_body]")
{
   auto arg = api::from_body<custom_type>{};
   auto req = make_str_req("/", "{\"value\":\"test string\"}", "application/json");
   auto result = parse_api_request(req, arg);
   REQUIRE(result == error_code{});
   REQUIRE(arg.value.value == "test string");
}

#endif