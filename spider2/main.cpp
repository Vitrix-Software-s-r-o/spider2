#include "spider2/spider2.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <iostream>

using namespace spider2;

class ws_event_handler : public websocket_event_handler
{
   inline virtual void on_accept_failed(const websocket_connection_data &data)
   {
      std::cout << "accept failed" << std::endl;
   }

   inline virtual void on_accept(const websocket_connection_data &data, websocket_connection_ptr const &connection)
   {
      std::cout << "connection accepted" << std::endl;
   }

   inline virtual void on_close(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                websocket_close_reason reason)
   {
      std::cout << "connection closed" << std::endl;
   }

   inline virtual void on_message(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                  std::string_view message)
   {
      std::cout << "message received: " << message << std::endl;
      connection->send_message("echo: "s + std::string{message});
   }

   inline virtual void on_exception(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                    std::exception_ptr ptr)
   {
      std::string ex_text;
      try
      {
         std::rethrow_exception(ptr);
      }
      catch (const std::exception &e)
      {
         ex_text = e.what();
      }
      catch (...)
      {
         ex_text = "unknown exception";
      }
      std::cout << "exception: " << ex_text << std::endl;
   };
};

int main()
{
   // get current executable folder
   const auto executable_directory_path = boost::dll::program_location().parent_path();

   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto ws_events = ws_event_handler{};
   auto app = app_context_base{.token = stop_src.get_token()};
   auto simple_app =
       begin_app() + on_scope("/ws", websocket_handler(ws_events)) +
       on_scope("/", static_files(executable_directory_path / "test-site", {.index_files = {"index.html"s}}));

   std::cout << "Starting server on http://localhost:3000" << std::endl;
   run_web_app(io_loop,
               {.tcp_listen = {tcp::endpoint{io::ip::address_v4::loopback(), 3000}},
                .connection_keep_alive = std::chrono::seconds{30}},
               app, simple_app);

   return 0;
}