//
// Created by jhrub on 20.02.2023.
//
#include <spider2/middleware/compress.h>
#include <spider2/routing/router.h>
#include <spider2/testing.h>

#include <catch.hpp>

using namespace spider2;

auto json_test = string{"{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}{\"name\":\"John\", \"age\":30, \"car\":null}"};
auto make_test_app()
{
   return begin_app() + on_get("/",
                               [](request &req) -> await_response
                               { co_return response::return_string(http::status::ok, json_test, "application/json"); });
}

TEST_CASE("compress middleware", "[compress]")
{
   auto router = make_test_app();
   auto request_pipeline = (begin_middleware() | compress_middleware{})(router);

   auto message = http::request<http::string_body>{};
   message.target("/");
   message.method(http::verb::get);
   message.set(http::field::accept_encoding, "br");
   std::cout << message[http::field::accept_encoding] << std::endl;

   auto response = testing::process_request(request_pipeline, message);
   REQUIRE(response.get_status() == status::ok);

   std::cout << "original size: " << json_test.size() << " compressed size: "<<response.try_get_string_body()->size();
   REQUIRE(response.try_get_string_body()->size() < json_test.size());
}