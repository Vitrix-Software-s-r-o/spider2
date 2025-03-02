#pragma once

#include "../types.h"
#include "spider2/types/structs/cancellation_timer.h"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <queue>

namespace spider2
{

   struct websocket_connection_context : public websocket_connection
   {
      inline static std::uint64_t get_next_id()
      {
         static std::atomic<std::uint64_t> id_counter = {};
         return id_counter++;
      }
      flat_buffer buffer;

      websocket_event_handler &handler;
      http::request<http::empty_body> request;

      websocket_connection_data data;
      websocket::stream<tcp::socket> websocket;

      message_queue outgoing_messages;
      std::stop_source stop_source = {};
      websocket_settings settings;

      websocket_close_reason close_reason = websocket_close_reason::close_normal;

      inline websocket_connection_context(websocket_event_handler &handler, http::request<http::empty_body> const &req,
                                          websocket::stream<tcp::socket> ws, const websocket_settings &settings)
          : handler(handler), request(req), data{.id = get_next_id(),
                                                 .url_endpoint = static_cast<std::string>(req.target()),
                                                 .fields = static_cast<const http::fields &>(req)},
            websocket{std::move(ws)}, outgoing_messages(websocket.get_executor()), settings(settings)
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

         if (ptr->settings.recv_timeout.has_value() && ptr->settings.recv_timeout.value() > std::chrono::seconds{0})
         {
            // start the time bounded read
            auto watchdog_timer =
                cancellation_timer(ptr->websocket.get_executor(), ptr->stop_source, ptr->settings.recv_timeout.value());
            auto io_watchdog =
                watchdog_timer.start_watchdog([ptr]() { ptr->close_reason = websocket_close_reason::recv_timeout; });

            while (!ptr->stop_source.stop_requested())
            {
               const auto [ec, len] = co_await ptr->websocket.async_read(ptr->buffer, use_tuple_awaitable);
               io_watchdog.stop();

               if (ec)
               {
                  ptr->close();
               }
               else
               {

                  ptr->handler.on_message(ptr->data, ptr,
                                          std::string_view{reinterpret_cast<char *>(ptr->buffer.data().data()), len});
                  ptr->buffer.consume(len);
               }
            }
         }
         else
         {
            // start unbounded read

            while (!ptr->stop_source.stop_requested())
            {
               const auto [ec, len] = co_await ptr->websocket.async_read(ptr->buffer, use_tuple_awaitable);

               if (ec)
               {
                  ptr->close();
               }
               else
               {

                  ptr->handler.on_message(ptr->data, ptr,
                                          std::string_view{reinterpret_cast<char *>(ptr->buffer.data().data()), len});
                  ptr->buffer.consume(len);
               }
            }
         }
      }

      static auto get_message_buffer(const std::variant<std::string, std::shared_ptr<std::string>> &msg)
          -> const std::string &
      {
         const std::string &msg_ref = std::visit(
             [](auto &v) -> const std::string &
             {
                using arg_type = std::decay_t<decltype(msg)>;
                if constexpr (std::is_same_v<arg_type, std::string>)
                {
                   return v;
                }
                else if constexpr (std::is_same_v<arg_type, std::shared_ptr<std::string>>)
                {
                   return *v;
                }
                else
                {
                   static std::string empty;
                   return empty;
                }
             },
             msg);
         return msg_ref;
      }
      static auto start_send(websocket_connection_context &ctx) -> io::awaitable<void>
      {
         if (ctx.settings.send_timeout.has_value())
         {
            // start the time bounded send
            auto watchdog_timer =
                cancellation_timer(ctx.websocket.get_executor(), ctx.stop_source, ctx.settings.send_timeout.value());

            while (!ctx.stop_source.stop_requested())
            {
               const auto message_to_send = co_await ctx.outgoing_messages.async_dequeue(ctx.stop_source.get_token());

               auto deadline =
                   watchdog_timer.start_watchdog([&ctx]() { ctx.close_reason = websocket_close_reason::send_timeout; });

               if (message_to_send.has_value())
               {
                  const auto &msg_ref = get_message_buffer(message_to_send.value());

                  const auto [ec, len] = co_await ctx.websocket.async_write(io::buffer(msg_ref), use_tuple_awaitable);
                  deadline.stop();
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
         }
         else
         {
            // start unbounded send
            while (!ctx.stop_source.stop_requested())
            {
               const auto message_to_send = co_await ctx.outgoing_messages.async_dequeue(ctx.stop_source.get_token());
               if (message_to_send.has_value())
               {
                  const auto &msg_ref = get_message_buffer(message_to_send.value());
                  const auto [ec, len] = co_await ctx.websocket.async_write(io::buffer(msg_ref), use_tuple_awaitable);

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
         }
      }

      static auto accept(std::shared_ptr<websocket_connection_context> ptr) -> void
      {
         using namespace boost::asio::experimental::awaitable_operators;
         ptr->websocket.set_option(websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
         ptr->websocket.async_accept(
             ptr->request,
             [ptr](const boost::system::error_code &ec)
             {
                if (ec)
                {
                   ptr->handler.on_accept_failed(ptr->data, ec);
                }
                else
                {
                   ptr->handler.on_accept(ptr->data, ptr);
                   io::co_spawn(
                       ptr->websocket.get_executor(),
                       [ptr]() -> io::awaitable<void>
                       {
                          return [](auto connection_ptr) -> io::awaitable<void>
                          {
                             const auto close_it = [connection_ptr]()
                             {
                                auto &socket = connection_ptr->websocket.next_layer();

                                if (socket.is_open())
                                {
                                   boost::system::error_code shutdown_ec = {};
                                   socket.shutdown(boost::asio::socket_base::shutdown_both, shutdown_ec);
                                   socket.close(shutdown_ec);
                                }
                             };

                             const auto watchdog =
                                 std::stop_callback{connection_ptr->stop_source.get_token(), [&]() { close_it(); }};

                             try
                             {
                                co_await (start_read(connection_ptr) || start_send(*connection_ptr));
                                if (connection_ptr->close_reason == websocket_close_reason::close_normal)
                                {
                                   const auto [close_ec] = co_await connection_ptr->websocket.async_close(
                                       websocket::close_code::normal, use_tuple_awaitable);
                                   ignore_unused(close_ec);
                                }
                             }
                             catch (...)
                             {
                                connection_ptr->close_reason = websocket_close_reason::exception;
                                connection_ptr->handler.on_exception(connection_ptr->data, connection_ptr,
                                                                     std::current_exception());
                             }

                             connection_ptr->handler.on_close(connection_ptr->data, connection_ptr,
                                                              connection_ptr->close_reason);
                          }(std::move(ptr));
                       },
                       io::detached);
                }
             });
      }
   };

   inline auto websocket_handler(websocket_event_handler &handler, websocket_settings settings = {})
   {
      return [&, settings](request &req) -> await_response
      {
         const auto *message = req.try_get_message<http::empty_body>();
         if (!message)
         {
            return []() -> await_response
            { co_return response::return_string(http::status::bad_request, "bad request"); }();
         }

         if (!websocket::is_upgrade(*message))
         {
            return []() -> await_response
            { co_return response::return_string(http::status::upgrade_required, "upgrade required"); }();
         }

         using connection_ptr_t = std::shared_ptr<websocket_connection_context>;
         auto ctx_ptr = std::make_shared<websocket_connection_context>(
             handler, *message, websocket::stream<tcp::socket>(req.steal_socket()), settings);

         return [](connection_ptr_t connection_ptr) -> await_response
         {
            websocket_connection_context::accept(std::move(connection_ptr));

            // socket is stolen nothing to write, so we return nothing
            co_return response::return_nothing();
         }(std::move(ctx_ptr));
      };
   }
} // namespace spider2