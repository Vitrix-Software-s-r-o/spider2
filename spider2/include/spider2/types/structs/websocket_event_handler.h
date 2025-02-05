//
// Created by jan on 9.12.24.
//

#pragma once
#include "websocket_connection.h"
namespace spider2
{
   class websocket_event_handler
   {
    public:
      virtual ~websocket_event_handler() = default;

      virtual void on_accept(const websocket_connection_data &data, websocket_connection_ptr const &connection){};
      virtual void on_close(const websocket_connection_data &data, websocket_connection_ptr const &connection) {};
      virtual void on_message(const websocket_connection_data &data, websocket_connection_ptr const &connection, std::string_view message){};

   };
} // namespace spider2