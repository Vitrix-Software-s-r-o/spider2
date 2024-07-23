#include <spider2/api/from_body.h>

#include <boost/json.hpp>
#include <catch.hpp>
#include <spider2/api/from_query.h>
#include <spider2/testing.h>

using namespace std::string_literals;
using namespace spider2;
namespace
{
   auto make_req(const string &path_with_query)
   {
      static app_context_base app_context;
      return request{http::request<http::empty_body>{verb::get, path_with_query, 11}, app_context};
   }

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

TEST_CASE("test parse query_args structure", "[query_args]")
{
   auto assert_query_args = [](auto &ec, auto &arg)
   {
      REQUIRE(ec == error_code{});
      REQUIRE(arg.value.query_map.size() == 5);

      auto number_value = arg.value.template as_value<int>("number");
      REQUIRE(number_value.value_or(0) == 123);

      auto string_value = arg.value.template as_value<string>("string");
      REQUIRE(string_value.value_or("") == "example string");

      auto vector_value = arg.value.template as_vector<int>("vector");
      REQUIRE(vector_value == vector<int>{1, 2, 3});

      auto str_vector_value = arg.value.template as_vector<string>("str_vector");
      REQUIRE(str_vector_value == vector<string>{"one", "two", "three"});

      auto multiple_arg_value = arg.value.template as_vector<int>("multiple_arg");
      REQUIRE(multiple_arg_value == vector<int>{1, 2, 3});
   };

   SECTION("from_query")
   {
      auto arg = api::from_query<api::query_args>{};
      auto req = make_req(
          "/?number=123&string=example%20string&vector=1,2,3&str_vector=one,two,three&multiple_arg=1&multiple_arg=2&"
          "multiple_arg=3");
      auto result = parse_api_request(req, arg);
      assert_query_args(result, arg);
   }

   SECTION("from url encoded body")
   {
      auto arg = api::from_body<api::query_args>{};

      auto req = make_str_req(
          "/?ignored_query=1",
          "number=123&string=example%20string&vector=1,2,3&str_vector=one,two,three&multiple_arg=1&multiple_arg=2&"
          "multiple_arg=3",
          "application/x-www-form-urlencoded");
      auto result = parse_api_request(req, arg);
      assert_query_args(result, arg);

      auto query_arg = api::from_query<api::query_args>{};
      result = parse_api_request(req, query_arg);

      REQUIRE(result == error_code{});
      REQUIRE(query_arg.value.as_value<bool>("ignored_query").value_or(false) == true);
   }

   SECTION("from json encoded body")
   {
      auto arg = api::from_body<api::query_args>{};

      auto req = make_str_req(
          "/?ignored_query=1",
          R"({"number":123,"string":"example string","vector":[1,2,3],"str_vector":["one","two","three"],"multiple_arg":[1,2,3]})"s,
          "application/json");
      auto result = parse_api_request(req, arg);
      assert_query_args(result, arg);

      auto query_arg = api::from_query<api::query_args>{};
      result = parse_api_request(req, query_arg);

      REQUIRE(result == error_code{});
      REQUIRE(query_arg.value.as_value<bool>("ignored_query").value_or(false) == true);
   }
}
