#include "spider2/spider2.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <iostream>

using namespace spider2;

int main()
{
   // get current executable folder
   const auto executable_directory_path = boost::dll::program_location().parent_path();

   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto app = app_context_base{.token = stop_src.get_token()};
   auto simple_app = begin_app() + on_scope("/", static_files(executable_directory_path / "test-site",
                                                              {.index_files = {"index.html"s}}));

   run_web_app(io_loop,
               {.tcp_listen = {tcp::endpoint{io::ip::address_v4::loopback(), 3000}},
                .connection_keep_alive = std::chrono::seconds{30}},
               app, simple_app);

   return 0;
}