#pragma once

#include "../types.h"
#include "spider2/types/structs/cancellation_timer.h"

#include <boost/asio/experimental/awaitable_operators.hpp>
#include <queue>

namespace spider2
{

   struct websocket_connection_context : public websocket_connection
   {

      static std::atomic<std::uint64_t> id_counter;
      flat_buffer buffer;

      websocket_event_handler &handler;
      websocket_connection_data data;
      websocket::stream<tcp::socket> websocket;

      message_queue outgoing_messages;
      std::stop_source stop_source = {};
      websocket_settings settings;

      websocket_close_reason close_reason = websocket_close_reason::close_normal;

      inline websocket_connection_context(websocket_event_handler &handler, endpoint_base ep, http::fields fields,
                                          websocket::stream<tcp::socket> ws, websocket_settings settings)
          : handler(handler),
            data{.id = id_counter++, .url_endpoint = static_cast<std::string>(ep.path), .fields = std::move(fields)},
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
         auto watchdog_timer = cancellation_timer(
             ptr->websocket.get_executor(), ptr->stop_source,
             ptr->settings.recv_timeout.has_value() ? ptr->settings.recv_timeout.value()
                                                    : std::numeric_limits<std::chrono::steady_clock::duration>::max());

         const auto watchdog =
             std::stop_callback{ptr->stop_source.get_token(), [ptr]()
                                {
                                   boost::system::error_code ec;
                                   ptr->websocket.next_layer().shutdown(boost::asio::socket_base::shutdown_receive, ec);
                                }};

         while (!ptr->stop_source.stop_requested())
         {
            auto io_watchdog =
                watchdog_timer.start_watchdog([ptr]() { ptr->close_reason = websocket_close_reason::recv_timeout; });
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

      static auto start_send(websocket_connection_context &ctx) -> io::awaitable<void>
      {
         auto watchdog_timer = cancellation_timer(
             ctx.websocket.get_executor(), ctx.stop_source,
             ctx.settings.recv_timeout.has_value() ? ctx.settings.recv_timeout.value()
                                                   : std::numeric_limits<std::chrono::steady_clock::duration>::max());

         while (!ctx.stop_source.stop_requested())
         {
            const auto message_to_send = co_await ctx.outgoing_messages.async_dequeue(ctx.stop_source.get_token());

            auto deadline =
                watchdog_timer.start_watchdog([&ctx]() { ctx.close_reason = websocket_close_reason::send_timeout; });

            if (message_to_send.has_value())
            {
               auto &msg = message_to_send.value();
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

               const auto watchdog = std::stop_callback{ctx.stop_source.get_token(), [&]()
                                                        {
                                                           boost::system::error_code ec;
                                                           ctx.websocket.next_layer().shutdown(
                                                               boost::asio::socket_base::shutdown_send, ec);
                                                        }};
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

      static auto accept(std::shared_ptr<websocket_connection_context> ptr) -> void
      {
         using namespace boost::asio::experimental::awaitable_operators;

         io::co_spawn(
             ptr->websocket.get_executor(),
             [ptr]() -> io::awaitable<void>
             {
                return [](std::shared_ptr<websocket_connection_context> connection_ptr) -> io::awaitable<void>
                {
                   const auto close_it = [connection_ptr]()
                   {
                      auto &socket = connection_ptr->websocket.next_layer();

                      boost::system::error_code shutdown_ec = {};
                      socket.shutdown(boost::asio::socket_base::shutdown_both, shutdown_ec);
                      socket.close(shutdown_ec);
                   };

                   try
                   {
                      const auto io_watchdog =
                          std::stop_callback{connection_ptr->stop_source.get_token(), [close_it]() { close_it(); }};

                      auto watchdog_timer =
                          cancellation_timer(connection_ptr->websocket.get_executor(), connection_ptr->stop_source,
                                             connection_ptr->settings.recv_timeout.has_value()
                                                 ? connection_ptr->settings.accept_timeout.value()
                                                 : std::chrono::minutes{1});

                      auto deadline_timer = watchdog_timer.start_watchdog();
                      const auto [ec] = co_await connection_ptr->websocket.async_accept(use_tuple_awaitable);
                      if (ec)
                      {
                         connection_ptr->handler.on_accept_failed(connection_ptr->data);
                         co_return;
                      }

                      deadline_timer.stop();

                      connection_ptr->handler.on_accept(connection_ptr->data, connection_ptr);

                      co_await (connection_ptr->start_read(connection_ptr) ||
                                connection_ptr->start_send(*connection_ptr));

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

                   close_it();
                   connection_ptr->handler.on_close(connection_ptr->data, connection_ptr, connection_ptr->close_reason);
                }(std::move(ptr));
             },
             io::detached);
      }
   };

   inline auto websocket(websocket_event_handler &handler, websocket_settings settings = {})
   {
      return [&, settings](request &req) -> await_response
      {
         using connection_ptr_t = std::shared_ptr<websocket_connection_context>;
         auto ctx_ptr = std::make_shared<websocket_connection_context>(
             handler, req.get_endpoint(), req.get_headers(), websocket::stream<tcp::socket>(req.steal_socket()),
             settings);

         return [](connection_ptr_t connection_ptr) -> await_response
         {
            websocket_connection_context::accept(std::move(connection_ptr));

            // socket is stolen nothing to write, so we return nothing
            co_return response::return_nothing();
         }(std::move(ctx_ptr));
      };
   }
} // namespace spider2