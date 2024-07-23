#include <boost/asio.hpp>
#include <boost/dll/config.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/json/src.hpp>
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <spider2/react.h>
#include <spider2/spider2.h>

using namespace spider2;
using namespace std::literals;

auto make_api()
{
   return begin_app() +
          on_get("/time",
                 []()
                 {
                    static auto counter = std::atomic<int>{};
                    auto current_counter = ++counter;

                    auto time = std::chrono::system_clock::now();
                    return response::return_string(http::status::ok,
                                                   fmt::format("Request {} Server time: {:%T}", current_counter, time),
                                                   "text/plain");
                 });
}

int main(int argc, char const *argv[])
{
   auto io_loop = io::io_context{};
   auto stop_src = std::stop_source{};

   auto app = app_context_base{.token = stop_src.get_token()};

   const auto executable_directory_path = boost::dll::program_location().parent_path();
   const auto debug_react = argc >= 2 ? argv[1] == "--debug-react"sv : false;

   const auto http_server_endpoint = tcp::endpoint{io::ip::address_v4::loopback(), 3001};

   auto application_router =
       begin_app() + on_scope("/api", make_api()) +
       on_scope("/", enable_react(react_config{
                         .debug_react = debug_react,
                         .react_index_html_path =
                             // find_debug_react_index_html is a helper function that searches for the index.html file
                             // in the react app under parent directories relative to the executable directory
                         debug_react
                             ? react::find_debug_react_index_html(
                                   executable_directory_path, "../spider2-react/examples/example_app/public/index.html")
                             : executable_directory_path / "react_build" / "index.html"}));

   run_web_app(io_loop, {.tcp_listen = {http_server_endpoint}, .connection_keep_alive = std::chrono::seconds{30}}, app,
               application_router);
   return 0;
}
