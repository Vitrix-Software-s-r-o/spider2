#include <catch.hpp>
#include <spider2/api.h>
#include <spider2/testing.h>

using namespace spider2;

struct custom_type
{
   std::string value;

   friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, custom_type const &rec)
   {
      auto &obj = jv.emplace_object();
      obj["value"] = rec.value;
   }

   friend auto tag_invoke(boost::json::value_to_tag<custom_type>, boost::json::value const &jv) -> custom_type
   {
      auto &obj = jv.as_object();
      return custom_type{obj.at("value").as_string().c_str()};
   }
};

namespace
{
   auto make_str_req(const std::string &path_with_query, const std::string &body, const std::string &content_type)
   {
      auto message = http::request<http::string_body>{verb::post, path_with_query, 11};
      message.body() = body;
      message.set(http::field::content_type, content_type);
      message.content_length(body.size());

      return message;
   }

   auto make_const_value_ref_api()
   {
      return begin_app() + on_api_post("/const_value_ref", [](api::from_body<custom_type> const &value)
      {
         return api::result{value.value};
      });
   };

   auto make_value_ref_api()
   {
      return begin_app() +
             on_api_post("/value_ref", [](api::from_body<custom_type> &value)
             {
                return api::result{value.value};
             });
   };

   auto make_value_api()
   {
      return begin_app() +
             on_api_post("/value", [](api::from_body<custom_type> value)
             {
                return api::result{value.value};
             });
   };
} // namespace

TEST_CASE("test supported types", "[args]")
{
   SECTION("const value ref")
   {
      auto app = make_const_value_ref_api();
      auto req = make_str_req("/const_value_ref", "{\"value\":\"const value ref\"}", "application/json");

      auto resp = testing::process_request(app, req);

      REQUIRE(resp.get_status() == http::status::ok);
      REQUIRE(resp.get_headers()->at(http::field::content_type) == "application/json");
      REQUIRE(resp.try_get_string_body().value_or("") == "{\"value\":\"const value ref\"}");
   }

   SECTION("value ref")
   {
      auto app = make_value_ref_api();
      auto req = make_str_req("/value_ref", "{\"value\":\"value ref\"}", "application/json");

      auto resp = testing::process_request(app, req);

      REQUIRE(resp.get_status() == http::status::ok);
      REQUIRE(resp.get_headers()->at(http::field::content_type) == "application/json");
      REQUIRE(resp.try_get_string_body().value_or("") == "{\"value\":\"value ref\"}");
   }

   SECTION("value")
   {
      auto app = make_value_api();
      auto req = make_str_req("/value", "{\"value\":\"value\"}", "application/json");

      auto resp = testing::process_request(app, req);

      REQUIRE(resp.get_status() == http::status::ok);
      REQUIRE(resp.get_headers()->at(http::field::content_type) == "application/json");
      REQUIRE(resp.try_get_string_body().value_or("") == "{\"value\":\"value\"}");
   }
}