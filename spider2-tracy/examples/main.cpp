#include <boost/asio.hpp>
#include <boost/json/src.hpp>
#include <chrono>
#include <spider2/spider2.h>
#include <spider2/tracy.h>
using namespace spider2;
using namespace std::literals;

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
          on_api_get("/path",
                     [](request &req, query_data_payload const &data, tracy_context &ctx) -> await_response
                     {
                        return [](request &req, query_data_payload const &data, tracy_context &ctx) -> await_response
                        {
                           auto exec = co_await io::this_coro::executor;
                           ctx.enter();

                           auto timer = io::steady_timer{exec};
                           timer.expires_after(2s);

                           ctx.suspend();
                           auto [ec] = co_await timer.async_wait(use_tuple_awaitable);

                           {
                              auto scope = ctx.enter_scope();
                              std::this_thread::sleep_for(1s);
                           }

                           if (!ec)
                           {
                              co_return response::return_string(http::status::ok, data.query);
                           }
                           else
                           {
                              co_return response::return_string(http::status::internal_server_error,
                                                                "request canceled");
                           }
                        }(req, data, ctx);
                     });
}

int main()
{
   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto app = app_context_base{.token = stop_src.get_token()};

   auto app_router = make_test_api_app();
   run_web_app(io_loop,
               {.tcp_listen = {tcp::endpoint{io::ip::address_v4::loopback(), 3000}},
                .connection_keep_alive = std::chrono::seconds{30}},
               app, (begin_middleware() | enable_tracy_profiler())(app_router));
   return 0;
}
