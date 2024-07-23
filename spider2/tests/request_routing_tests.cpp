#include <catch.hpp>
#include <spider2/testing.h>

using namespace spider2;

auto make_test_router_app()
{
   return begin_app() +
          on_get("/",
                 [](request &req) -> await_response
                 {
                    co_return response::return_string(http::status::ok,
                                                      static_cast<std::string>(req.get_processing_endpoint().path));
                 }) +
          on_post("/",
                  [](request &req) -> await_response { co_return response::return_string(http::status::ok, "post"); }) +
          on_get("/sync",
                 [](request &req) -> response { return response::return_string(http::status::ok, "sync response"); });
}

TEST_CASE("test router", "[router]")
{
   auto router = make_test_router_app();

   SECTION("success match")
   {
      auto message = http::request<http::string_body>{};
      message.target("/");
      message.method(http::verb::get);

      auto response = testing::process_request(router, std::move(message));
      REQUIRE(response.get_status() == status::ok);
   }
   /*
      SECTION("success match on sync")
      {
         auto message = http::request<http::string_body>{};
         message.target("/sync");
         message.method(http::verb::get);

         auto response = testing::process_request(router, std::move(message));
         REQUIRE(response.get_status() == status::ok);
         REQUIRE(response.try_get_string_body().value_or(spider2::string_view{}) == "sync response");
      }*/

   SECTION("not found")
   {
      auto message = http::request<http::string_body>{};
      message.target("/not-existing");
      message.method(http::verb::get);

      auto response = testing::process_request(router, std::move(message));
      REQUIRE(response.get_status() == status::not_found);
   }

   SECTION("success on nested router")
   {
      auto router_with_nested = router.on_path("/nested", make_test_router_app());
      auto message = http::request<http::string_body>{};
      message.target("/");
      message.method(http::verb::get);

      {
         response resp = testing::process_request(router_with_nested, message);
         auto body = resp.try_get_string_body().value_or(spider2::string_view{});
         REQUIRE(body == "/");

         REQUIRE(resp.get_status() == status::ok);
      }

      message.target("/nested/");
      {
         auto response = testing::process_request(router_with_nested, message);
         REQUIRE(response.get_status() == status::ok);
         auto body = response.try_get_string_body().value_or(spider2::string_view{});
         REQUIRE(body == "/");
      }

      message.target("/nested");
      {
         auto response = testing::process_request(router_with_nested, message);
         REQUIRE(response.get_status() == status::not_found);
      }

      message.target("/nested/not-existing");
      {
         auto response = testing::process_request(router_with_nested, message);
         REQUIRE(response.get_status() == status::not_found);
      }

      message.target("/not-existing");
      {
         auto response = testing::process_request(router_with_nested, message);
         REQUIRE(response.get_status() == status::not_found);
      }
   }
}

struct custom_context
{
   int x = 100;
};

struct other_context
{
   int x = 100;
};
const auto context_middleware = [](auto fun)
{
   return [=](request &req, auto &&...ctx) -> await_response
   {
      const auto new_context = custom_context{};
      co_return (co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)..., new_context));
   };
};

const auto other_context_middleware = [](auto fun)
{
   return [=](request &req, auto &&...ctx) -> await_response
   {
      const auto new_context = other_context{};
      co_return (co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)..., new_context));
   };
};

auto make_custom_context_test_app()
{
   return begin_app() + on_get("/", [](request &req, custom_context const &ctx, other_context const &) -> await_response
                               { co_return response::return_string(http::status::ok, fmt::format("x={}", ctx.x)); });
}

TEST_CASE("is router compatible with  middleware", "[router]")
{
   auto router = make_custom_context_test_app();
   auto request_pipeline = (begin_middleware() | context_middleware | other_context_middleware)(router);

   auto message = http::request<http::string_body>{};
   message.target("/");
   message.method(http::verb::get);

   auto response = testing::process_request(request_pipeline, message);
   REQUIRE(response.get_status() == status::ok);
   REQUIRE(response.try_get_string_body().value_or(spider2::string_view{}) == "x=100");
}

TEST_CASE("test app scoping", "[router]")
{
   auto router = begin_app() +
                 on_scope("/scope", begin_middleware() | context_middleware | other_context_middleware,
                          make_custom_context_test_app()) +
                 on_scope("/ignore-scope-args", begin_middleware() | context_middleware | other_context_middleware,
                          make_test_router_app());

   auto message = http::request<http::string_body>{};
   message.method(http::verb::get);

   SECTION("hitting the scope")
   {
      message.target("/scope/");
      auto response = testing::process_request(router, message);
      REQUIRE(response.get_status() == status::ok);
      REQUIRE(response.try_get_string_body().value_or(spider2::string_view{}) == "x=100");
   }

   SECTION("ignoring scope arguments if they are present")
   {
      message.target("/ignore-scope-args/");
      auto response = testing::process_request(router, message);
      auto body = response.try_get_string_body().value_or(spider2::string_view{});

      REQUIRE(response.get_status() == status::ok);
      REQUIRE(body == "/");
   }
}