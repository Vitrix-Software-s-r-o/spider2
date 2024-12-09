#pragma once

#include "../types.h"

#include <queue>

namespace spider2
{
   using outgoing_message = std::variant<std::string, std::shared_ptr<string>>;

   template <typename T>
   using is_outgoing_message = std::is_same<T, outgoing_message>;

   template <typename T>
   concept outgoing_message_concept = is_outgoing_message<T>::value;

   class message_queue
   {
    public:
      void queue(outgoing_message_concept auto &&msg)
      {
         std::lock_guard<std::mutex> lock(lock_);
         queue_.push(std::forward<std::decay_t<decltype(msg)>>(msg));
      }

      auto dequeue() -> std::optional<outgoing_message>
      {
         std::lock_guard<std::mutex> lock(lock_);
         if (queue_.empty())
         {
            return std::optional<outgoing_message>{};
         }

         auto msg = queue_.front();
         queue_.pop();

         return msg;
      }

    private:
      mutable std::mutex lock_;
      std::queue<outgoing_message> queue_;
   };

   struct websocket_connection_context
   {
      endpoint_base endpoint;
      http::fields fields;
      websocket::stream<tcp::socket> websocket;

      message_queue outgoing_messages;

      inline websocket_connection_context(endpoint_base ep, http::fields fields, websocket::stream<tcp::socket> ws)
          : endpoint{ep}, fields{fields}, websocket{std::move(ws)}
      {
         websocket.set_option(websocket::stream_base::timeout::suggested(role_type::server));
      }
   };

   using websocket_connection_ptr = std::shared_ptr<websocket_connection_context>;

   inline auto websocket()
   {
      return [](request &req) -> await_response
      {
         auto connection_ptr = std::make_shared<websocket_connection_context>(websocket_connection_context{
             req.get_endpoint(), req.get_headers(), websocket::stream<tcp::socket>(req.steal_socket())});

         const auto [ec] = co_await connection_ptr->websocket.async_accept(use_tuple_awaitable);
         if (!ec)
         {
         }
      };
   }
} // namespace spider2