#include <boost/json.hpp>
#include <catch.hpp>
#include <spider2/api/op_result.h>
#include <spider2/testing.h>

using namespace std::string_literals;
using namespace spider2;
namespace
{
   auto dummy_request = []()
   {
      static app_context_base app_context;
      return request{http::request<http::empty_body>{verb::get, "/", 11}, app_context};
   }();
} // namespace

TEST_CASE("test op_result serialization", "[op_result]")
{
   auto result_message = api::op_result<>::make_success_result({{"test string"s}});
   static_assert(api::is_api_result_v<decltype(result_message)>);

   auto resp = format_api_response(dummy_request, std::move(result_message));
   auto *fields = resp.get_headers();
   REQUIRE(fields != nullptr);
   REQUIRE(fields->at(http::field::content_type) == "application/json");
   REQUIRE(resp.get_status() == http::status::ok);
   REQUIRE(resp.try_get_string_body().value_or("") ==
           "{\"value\":\"test string\",\"message\":{\"message\":\"T:operation.success\",\"tokens\":[]}}");
}

TEST_CASE("test op_result validation failed serialization", "[op_result]")
{
   auto result_message = api::op_result<>::make_validation_failed_result({{"test string"s, {}, {}, "field"}});
   static_assert(api::is_api_result_v<decltype(result_message)>);

   auto resp = format_api_response(dummy_request, std::move(result_message));
   auto *fields = resp.get_headers();
   REQUIRE(fields != nullptr);
   REQUIRE(fields->at(http::field::content_type) == "application/json");
   REQUIRE(resp.get_status() == http::status::expectation_failed);
   REQUIRE(
       resp.try_get_string_body().value_or("") ==
       R"({"message":{"message":"T:operation.validation.failed","tokens":[]},"field_messages":[{"message":"test string","tokens":[],"field_id":"field"}]})");
}