#include "spider2/spider2.h"
#include <boost/dll/runtime_symbol_info.hpp>
#include <iostream>

using namespace spider2;

class ws_event_handler : public websocket_event_handler
{
 public:
   inline virtual void on_accept_failed(const websocket_connection_data &data, boost::system::error_code const &ec)
   {
      std::cout << "accept failed: " << ec.message() << std::endl;
   }

   inline virtual void on_accept(const websocket_connection_data &data, websocket_connection_ptr const &connection)
   {
      std::cout << "connection accepted" << std::endl;
      std::lock_guard lk{m_};
      connections_[data.id] = connection;
   }

   inline virtual void on_close(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                websocket_close_reason reason)
   {
      std::cout << "connection closed" << std::endl;
      std::lock_guard lk{m_};
      connections_.erase(data.id);
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

   void broadcast_message(const std::string &message) const
   {
      auto connections = [this]()
      {
         std::vector<websocket_connection_ptr> conns;
         std::lock_guard lk{m_};
         std::ranges::copy(connections_ | std::views::values, std::back_inserter(conns));

         return conns;
      }();

      auto shared_message = std::make_shared<std::string>(message);
      for (auto &conn : connections)
      {
         conn->send_message(shared_message);
      }
   }

 private:
   mutable std::mutex m_;
   std::unordered_map<std::uint64_t, websocket_connection_ptr> connections_;
};

int main()
{
   // get current executable folder
   const auto executable_directory_path = boost::dll::program_location().parent_path();

   auto io_loop = io::io_context{};

   auto stop_src = std::stop_source{};

   auto ws_events = ws_event_handler{};

   auto timer_thread = std::jthread{[&ws_events, token = stop_src.get_token()]()
                                    {
                                       int i = 0;
                                       while (!token.stop_requested())
                                       {
                                          i++;
                                          ws_events.broadcast_message(fmt::to_string(i));
                                          std::this_thread::sleep_for(std::chrono::milliseconds{16});
                                       }
                                    }};

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