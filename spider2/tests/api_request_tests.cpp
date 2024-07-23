#include <catch.hpp>
#include <spider2/routing.h>
#include <spider2/testing.h>
using namespace spider2;

struct query_data_payload
{
   string query;

   friend auto tag_invoke(tag_t<parse_api_request>, request &req, query_data_payload &data) -> error_code
   {
      auto result = error_code{};

      data.query = static_cast<string>(req.get_query_string());

      return result;
   }
};

struct data_payload
{
   string body;

   friend auto tag_invoke(tag_t<parse_api_request>, request &req, data_payload &data) -> error_code
   {
      auto result = error_code{};

      auto *message = req.try_get_message<http::string_body>();
      if (message != nullptr)
      {
         data.body = message->body();
      }
      else
      {
         result = make_error_code(request_error_code::api_input_data_error);
      }

      return result;
   }
};

struct custom_response_type
{
   string body;

   friend auto tag_invoke(tag_t<format_api_response>, custom_response_type &value, request &req) -> response
   {
      return response::return_string(http::status::ok, value.body);
   }
};

auto make_test_api_app()
{
   return begin_app() +
          on_api_get("/path", [](request &req, query_data_payload data) -> await_response
                     { co_return response::return_string(http::status::ok, data.query); }) +
          on_api_post("/", [](request &req, data_payload data) -> await_response
                      { co_return response::return_string(http::status::ok, data.body); });
}

TEST_CASE("test api router", "[router]")
{
   auto router = make_test_api_app();
   {
      auto message = http::request<http::string_body>{};
      message.target("/path?query=123");
      message.method(verb::get);

      auto response = testing::process_request(router, std::move(message));
      REQUIRE(response.get_status() == status::ok);
      auto body_result = response.try_get_string_body().value_or(std::string_view{});

      REQUIRE(body_result == "query=123");
   }

   {
      auto message = http::request<http::string_body>{};
      message.target("/");
      message.method(verb::post);
      message.body() = "test body";
      message.content_length(message.body().size());

      auto response = testing::process_request(router, std::move(message));
      REQUIRE(response.get_status() == status::ok);
      auto body_result = response.try_get_string_body().value_or(std::string_view{});

      REQUIRE(body_result == "test body");
   }
}