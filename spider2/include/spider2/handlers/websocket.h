#pragma once

#include "../types.h"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <queue>
namespace spider2
{

   struct websocket_connection_context : public websocket_connection
   {

      static std::atomic<std::uint64_t> id_counter;

      websocket_event_handler &handler;
      websocket_connection_data data;
      websocket::stream<tcp::socket> websocket;

      message_queue outgoing_messages = {};
      std::stop_source stop_source = {};
      vector<char> buffer = {};

      inline websocket_connection_context(websocket_event_handler &handler, endpoint_base ep, http::fields fields,
                                          websocket::stream<tcp::socket> ws)
          : handler(handler),
            data{.id = id_counter++, .url_endpoint = static_cast<std::string>(ep.path), .fields = std::move(fields)},
            websocket{std::move(ws)}
      {
         websocket.set_option(websocket::stream_base::timeout::suggested(role_type::server));
         buffer.reserve(4096);
      }

      inline auto get_data() const -> const websocket_connection_data & final
      {
         return data;
      }

      inline auto send_message(outgoing_message msg) -> void final
      {
         outgoing_messages.queue(std::move(msg));
      }

      inline auto close() -> void final
      {
         ignore_unused(stop_source.request_stop());
      }

      static auto start_read(std::shared_ptr<websocket_connection_context> ptr) -> io::awaitable<void>
      {
         while (!ptr->stop_source.stop_requested())
         {
            const auto [ec, len] = co_await ptr->websocket.async_read(io::buffer(ptr->buffer), use_tuple_awaitable);

            if (ec)
            {
               ptr->close();
            }
            else
            {
               ptr->handler.on_message(ptr->data, ptr, std::string_view{ptr->buffer.data(), len});
            }
         }
      }

      static auto start_send(websocket_connection_context &ctx) -> io::awaitable<void>
      {
         while (!ctx.stop_source.stop_requested())
         {
            const auto message_to_send = co_await ctx.outgoing_messages.async_dequeue(ctx.stop_source.get_token());

            if (message_to_send.has_value())
            {
               if (const auto *msg_ptr = std::get_if<string>(message_to_send.value()); msg_ptr)
               {
                  const auto watchdog = std::stop_callback{ctx.stop_source.get_token(),
                                                           [&]() { ctx.websocket.next_layer().shutdown_send(); }};
                  const auto [ec, len] = co_await ctx.websocket.async_write(io::buffer(*msg_ptr), use_tuple_awaitable);

                  if (ec)
                  {
                     ctx.close();
                  }
               }
               else if (const auto *msg_ptr = std::get_if<std::shared_ptr<string>>(message_to_send.value()); msg_ptr)
               {
                  const auto watchdog = std::stop_callback{ctx.stop_source.get_token(),
                                                           [&]() { ctx.websocket.next_layer().shutdown_send(); }};

                  const auto [ec, len] =
                      co_await ctx.websocket.async_write(io::buffer(*msg_ptr->get()), use_tuple_awaitable);

                  if (ec)
                  {
                     ctx.close();
                  }
               }
               else
               {
                  ctx.close();
               }
            }
            else
            {
               ctx.close();
            }
         }
      }

      static auto accept(std::shared_ptr<websocket_connection_context> ptr) -> void
      {
         using namespace boost::asio::experimental::awaitable_operators;

         io::co_spawn(
             ptr->websocket.get_executor(),
             [ptr]() -> io::awaitable<void>
             {
                return [](std::shared_ptr<websocket_connection_context> connection_ptr)
                {
                   const auto io_watchdog = std::stop_callback{connection_ptr->stop_source.get_token(), [&]()
                                                               {
                                                                  auto &socket = connection_ptr->websocket.next_layer();
                                                                  socket.shutdown_both();
                                                               }};

                   const auto [ec] = co_await connection_ptr->websocket.async_accept(use_tuple_awaitable);
                   if (ec)
                   {
                      co_return;
                   }

                   connection_ptr->handler.on_accept(connection_ptr->data, connection_ptr);

                   auto read_task = connection_ptr->start_read(connection_ptr);
                   auto write_task = connection_ptr->start_send(*connection_ptr);

                   co_await (read_task || write_task);

                   const auto [close_ec] = co_await connection_ptr->websocket.async_close(websocket::close_code::normal,
                                                                                          use_tuple_awaitable);
                   ptr->handler.on_close(ptr->data, ptr);

                   ignore_unused(close_ec);
                }(std::move(ptr));
             },
             io::detached);
      }
   };

   inline auto websocket(websocket_event_handler &handler)
   {
      return [&](request &req) -> await_response
      {
         using connection_ptr_t = std::shared_ptr<websocket_connection_context>;
         auto ctx_ptr = std::make_shared<websocket_connection_context>(
             handler, req.get_endpoint(), req.get_headers(), websocket::stream<tcp::socket>(req.steal_socket()));

         return [](connection_ptr_t connection_ptr) -> await_response
         {
            websocket_connection_context::accept(std::move(connection_ptr));

            co_return response::return_nothing();
         }(std::move(ctx_ptr));
      };
   }
} // namespace spider2