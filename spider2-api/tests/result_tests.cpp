#include <boost/json.hpp>
#include <catch.hpp>
#include <spider2/api/result.h>
#include <spider2/testing.h>

using namespace spider2;
namespace json = boost::json;

struct custom_type
{
   string value;

   friend void tag_invoke(json::value_from_tag, json::value &jv, custom_type const &c)
   {
      jv = {{"value", c.value}};
   }
};

namespace
{
   auto dummy_request = []()
   {
      static app_context_base app_context;
      return request{http::request<http::empty_body>{verb::get, "/", 11}, app_context};
   }();
} // namespace
TEST_CASE("test custom_response with Boost.JSON", "[result]")
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

struct custom_response_with_custom_formatter
{
   string value;

   friend auto tag_invoke(tag_t<api::format_result_value>, api::default_formatter_tag, request &,
                          api::result<custom_response_with_custom_formatter> const &c) -> string
   {
      return c.value.value;
   }
};

TEST_CASE("test custom_response_with_custom_formatter", "[result]")
{
   auto result_message = api::result{custom_response_with_custom_formatter{.value = "test string"}};
   static_assert(api::is_api_result_v<decltype(result_message)>);

   auto resp = format_api_response(dummy_request, std::move(result_message));
   auto *fields = resp.get_headers();
   REQUIRE(fields != nullptr);
   REQUIRE(fields->at(http::field::content_type) == "application/json");
   REQUIRE(resp.get_status() == http::status::ok);
   REQUIRE(resp.try_get_string_body().value_or("") == "test string");
}