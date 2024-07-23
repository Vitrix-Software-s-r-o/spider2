#include <spider2/spider2.h>
#include <spider2/testing.h>

#include <catch.hpp>

struct query_data_payload : tag_parse_query
{
   int b;
};
VISITABLE_STRUCT(query_data_payload, b);

struct data_payload
{
   int a;
};

VISITABLE_STRUCT(data_payload, a);
using namespace std::literals;

namespace
{
   auto make_test_slow_api_app()

   {
      return begin_app() +
             on_api_get("/",
                        [](request &req, query_data_payload data) -> await_response
                        {
                           // this is a slow operation
                           std::this_thread::sleep_for(250ms);
                           co_return response::return_string(
                               http::status::ok, static_cast<std::string>(req.get_processing_endpoint().path));
                        });
   }

   auto test_request() -> io::awaitable<std::tuple<spider2::error_code, http::status>>
   {
      auto executor = co_await io::this_coro::executor;

      auto connection = tcp::socket{executor};
      auto [ec] = co_await connection.async_connect(tcp::endpoint{io::ip::address_v4::loopback(), 3000},
                                                    ioe::as_tuple(io::use_awaitable));
      if (ec)
      {
         co_return std::tuple{ec, http::status::unknown};
      }

      auto message = http::request<http::string_body>{http::verb::get, "/", 11};
      auto [write_ec, written_bytes] =
          co_await http::async_write(connection, message, ioe::as_tuple(io::use_awaitable));

      if (write_ec)
      {
         co_return std::tuple{write_ec, http::status::unknown};
      }

      auto buffer = boost::beast::flat_buffer{};
      auto response = http::response<http::string_body>{};
      auto [read_ec, read_bytes] =
          co_await http::async_read(connection, buffer, response, ioe::as_tuple(io::use_awaitable));
      if (read_ec)
      {
         co_return std::tuple{read_ec, http::status::unknown};
      }

      co_return std::tuple{error_code{}, response.result()};
   };
} // namespace

TEST_CASE("test many parallel connections and long running operation", "[server]")
{
   using namespace spider2;

   auto cts = std::stop_source{};
   auto app = app_context_base{.token = cts.get_token()};

   auto web_app_thread = std::jthread(
       [&]()
       {
          auto io_loop = io::io_context{};

          auto slow_app = make_test_slow_api_app();

          run_web_app(io_loop, {{tcp::endpoint{io::ip::address_v4::loopback(), 3000}}}, app, slow_app);
       });

   app.lifecycle_indicator.wait_for_all_threads_started();

   auto io_loop = io::io_context{};
   auto responses = std::array<std::optional<std::tuple<spider2::error_code, http::status>>, 100>{};

   for (int i = 0; i != 100; ++i)
   {
      co_spawn(
          io_loop,
          [i, &responses]() -> io::awaitable<void>
          {
             responses[i] = co_await test_request();
             co_return;
          },
          io::detached);
   }

   io_loop.run();
   cts.request_stop();

   for (auto &resp : responses)
   {
      REQUIRE(resp.has_value());
      auto [ec, status] = resp.value();
      if (ec)
      {
         std::cout << ec.message() << std::endl;
      }
      REQUIRE(ec == error_code{});
      REQUIRE(status == http::status::ok);
   }
}