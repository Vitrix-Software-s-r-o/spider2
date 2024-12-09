//
// Created by jan on 9.12.24.
//

#pragma once
#include "../platform.h"
#include <cstdint>

namespace spider2
{
   struct websocket_connection_data
   {
      std::uint64_t id;
      std::string url_endpoint;
      http::fields fields;
   };

   class websocket_connection
   {
    public:
      virtual ~websocket_connection() = default;

      virtual auto send_message(std::string message) -> void = 0;
      virtual auto close() -> void = 0;
   };

   using websocket_connection_ptr = std::shared_ptr<websocket_connection>;
} // namespace spider2