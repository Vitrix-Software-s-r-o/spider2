//
// Created by jan on 9.12.24.
//

#pragma once
#include "websocket_connection.h"
namespace spider2
{
   enum class websocket_close_reason
   {
      close_normal,
      recv_timeout,
      send_timeout
   };

   class websocket_event_handler
   {
    public:
      virtual ~websocket_event_handler() = default;

      inline virtual void on_accept_failed(const websocket_connection_data &data) {};

      inline virtual void on_accept(const websocket_connection_data &data, websocket_connection_ptr const &connection) {
      };

      inline virtual void on_close(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                   websocket_close_reason reason) {};

      inline virtual void on_message(const websocket_connection_data &data, websocket_connection_ptr const &connection,
                                     std::string_view message) {};
   };
} // namespace spider2