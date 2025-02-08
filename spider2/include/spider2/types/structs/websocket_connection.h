//
// Created by jan on 9.12.24.
//

#pragma once
#include "../platform.h"
#include <cstdint>

namespace spider2
{
   struct websocket_settings
   {
      /// this will require to receive message on the sockets at least that often, default value is never
      std::optional<std::chrono::steady_clock::duration> recv_timeout = std::nullopt;
      /// this will require to send message within this timeout period, default value is never
      std::optional<std::chrono::steady_clock::duration> send_timeout = std::nullopt;

      /// this will require accept to finish within timeout (the default value is 1 minute)
      std::optional<std::chrono::steady_clock::duration> accept_timeout = std::nullopt;
   };

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

      [[nodiscard]]
      virtual auto get_data() const -> const websocket_connection_data & = 0;
      virtual auto send_message(outgoing_message message) -> void = 0;
      virtual auto close() -> void = 0;
   };

   using websocket_connection_ptr = std::shared_ptr<websocket_connection>;
} // namespace spider2