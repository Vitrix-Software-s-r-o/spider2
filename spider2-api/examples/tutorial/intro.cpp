#include <spider2/api/from_body/from_body.h>
#include <spider2/api/op_result/op_result.h>
#include <spider2/middleware/cors_middleware.h>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include <spider2/api.h>

#include <spider2/spider2.h>
using namespace boost::program_options;
using namespace spider2;

auto make_simple_app()
{
   return begin_app() +
          on_get("/query/",
                 [](request const &r)
                 {
                    // this endpoint will return the query string as part of its response

                    auto query = r.get_query_string();
                    return response::return_string(
                        status::ok, std::format(R"(<doctype html><html><body><h1>Query: {}</h1></body></html>)", query),
                        "text/html");
                 }) +
          on_post("/body/",
                  [](request &r) -> await_response
                  {
                     // this endpoint will return the body of the request as part of its response

                     // read the body of the request
                     const auto ec = co_await r.read_body<http::string_body>();
                     if (auto *message = r.try_get_message<http::string_body>(); !ec && message != nullptr)
                     {
                        // get the body of the message (Beast message with string_body)
                        auto &message_body = message->body();

                        // return the response object
                        co_return response::return_string(
                            status::ok,
                            std::format(R"(<doctype html><html><body><h1>Body: {}</h1></body></html>)", message_body),
                            "text/plain");
                     }
                     else
                     {
                        // return a bad request response
                        co_return response::return_string(status::bad_request, "Failed to read body", "text/plain");
                     }
                  });
}

int main(int argc, char **argv)
{

   auto desc = options_description("intro");
   desc.add_options()("help", "produce help message")("port", value<std::uint16_t>()->default_value(3001), "port")(
       "host", value<std::string>()->default_value("127.0.0.1"), "bind host")("debug",
                                                                              "loads resources from source location");

   auto parsed = parse_command_line(argc, argv, desc);

   auto vm = variables_map{};
   store(parse_command_line(argc, argv, desc), vm);
   notify(vm);

   if (vm.count("help"))
   {
      std::cout << desc << std::endl;
      return 1;
   }

   auto bind_ip = io::ip::address::from_string(vm.at("host").as<string>());
   auto bind_port = vm.at("port").as<std::uint16_t>();

   // you want to compose your app using the `begin_app` function
   auto complete_app =
       begin_app() +
       on_get("/", [](request &r) { return response::return_string(status::ok, "Hello, World!", "text/plain"); }) +
       on_scope("/api", make_simple_app());

   // namespace spider2::io a namespace alias for boost::asio
   auto io_loop = io::io_context{};
   auto stop_src = std::stop_source{};
   auto app = app_context_base{.token = stop_src.get_token()};

   std::cout << std::format("HTTP listening on {}:{}", bind_ip.to_string(), bind_port) << std::endl;
   run_web_app(
       io_loop,
       {
           .tcp_listen = {tcp::endpoint{bind_ip, bind_port}},
           .thread_pool_threads = 0,
           .io_threads = 1,
           .connection_keep_alive = std::chrono::seconds{30},
       },
       app,
       // we wrap the app in the processing pipeline where we add the cors middleware to allow cross origin requests
       (begin_middleware() | cors_middleware())(complete_app));

   // the app will run until the stop source is requested to stop (in this case that is forever)
}