#include <boost/asio.hpp>
#include <boost/json/src.hpp>
#include <chrono>
#include <spider2/spider2.h>
#include <spider2/proxy.h>
using namespace spider2;
using namespace std::literals;



int main()
{
   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto app = app_context_base{.token = stop_src.get_token()};

   auto app_router = naive_proxy_pass({.target = {.endpoint = tcp::endpoint{io::ip::address_v4::loopback(), 3000}}});
   run_web_app(io_loop,
               {.tcp_listen = {tcp::endpoint{io::ip::address_v4::loopback(), 3001}},
                .connection_keep_alive = std::chrono::seconds{30}},
               app, app_router);
   return 0;
}
