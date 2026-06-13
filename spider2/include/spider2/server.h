//
// Created by jhrub on 08.10.2022.
//

#pragma once

#include "asio.h"
#include "types.h"
#include <fmt/format.h>
#include <iostream>

namespace spider2
{

   /// Maximum time given to the watchdog's closing 408 write before the socket is force-closed.
   /// Prevents wedged peers (full TCP send buffer / half-open TCP) from pinning a file descriptor forever.
   inline constexpr auto kWatchdogCloseWriteTimeout = std::chrono::seconds{5};

   /// Initial backoff after an accept failure. Doubles up to kAcceptBackoffMax.
   inline constexpr auto kAcceptBackoffMin = std::chrono::milliseconds{100};
   inline constexpr auto kAcceptBackoffMax = std::chrono::seconds{5};

   io::awaitable<void> handle_connection(app_context_base &context, server_config const &config, tcp::socket socket,
                                         auto &&handler)
   {
      // std::cout << "start" << std::endl;;
      bool is_socket_stolen = false;

      auto buffer = flat_buffer{};
      for (;;)
      {
         auto request_obj = request{socket, buffer, context};
         auto executor = socket.get_executor();
         auto keep_alive_watchdog = cancel_after_timeout(
             executor,
             [&]()
             {
                struct local_handler
                {
                   tcp::socket socket_;
                   http::response<http::empty_body> message_;
                   io::steady_timer write_deadline_;
                   bool write_completed_ = false;
                   bool write_deadline_expired_ = false;

                   explicit local_handler(tcp::socket &&socket)
                       : socket_(std::move(socket)), write_deadline_(socket_.get_executor())
                   {
                      auto close_ec = error_code{};
                      // this will stop receiving of message
                      socket_.shutdown(boost::asio::socket_base::shutdown_receive, close_ec);
                      message_.keep_alive(false);
                      message_.result(http::status::request_timeout);
                   }

                   void finish_close()
                   {
                      // finish up the connection close
                      auto close_ec = error_code{};
                      socket_.shutdown(boost::asio::socket_base::shutdown_both, close_ec);
                      socket_.close(close_ec);
                   }
                };

                auto handler_ptr = std::make_shared<local_handler>(std::move(socket));

                // Bounded write: if the peer is wedged and async_write never completes,
                // this timer force-closes the socket so its file descriptor is released.
                handler_ptr->write_deadline_.expires_after(kWatchdogCloseWriteTimeout);
                handler_ptr->write_deadline_.async_wait(
                    [handler_ptr](error_code const &timer_ec)
                    {
                       if (timer_ec)
                       {
                          return; // timer cancelled (write completed first)
                       }
                       if (handler_ptr->write_completed_)
                       {
                          return;
                       }
                       handler_ptr->write_deadline_expired_ = true;
                       auto close_ec = error_code{};
                       handler_ptr->socket_.cancel(close_ec); // abort pending async_write
                       handler_ptr->finish_close();
                    });

                // We send here 408 - to indicate connection close
                // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/408
                http::async_write(handler_ptr->socket_, handler_ptr->message_,
                                  [handler_ptr](error_code const &, size_t) mutable
                                  {
                                     handler_ptr->write_completed_ = true;
                                     handler_ptr->write_deadline_.cancel();
                                     if (!handler_ptr->write_deadline_expired_)
                                     {
                                        handler_ptr->finish_close();
                                     }
                                  });
             },
             config.connection_keep_alive);

         auto ec = co_await request_obj.read_header();
         keep_alive_watchdog.cancel();

         if (ec)
         {
            break;
         }

         auto response_message = co_await handler(request_obj);
         if (is_socket_stolen = request_obj.is_socket_stolen(); is_socket_stolen || !socket.is_open())
         {
            break;
         }

         const auto request_keep_alive = request_obj.has_keep_alive();

         response_message.keep_alive =
             request_keep_alive ? config.connection_keep_alive : optional<std::chrono::steady_clock::duration>{};

         if (auto read_body_ec = co_await request_obj.read_body<http::string_body>())
         {
            break;
         }

         if (auto [write_body_ec, write_body_bytes] = co_await response_message.async_write(socket); write_body_ec)
         {
            break;
         }

         if (!request_keep_alive)
         {
            break;
         }
      }

      if (context.event_handler != nullptr && !is_socket_stolen)
      {
         auto local_ec = error_code{};
         auto remote_ec = error_code{};
         auto local_ep = socket.local_endpoint(local_ec);
         auto remote_ep = socket.remote_endpoint(remote_ec);
         context.event_handler->on_connection_event(server_event_handler::connection_event{
             .event_type = server_event_handler::connection_event::event::connection_closed,
             .accept_endpoint = local_ep,
             .client_endpoint = remote_ec ? std::nullopt : std::optional{remote_ep}});
      }
   }

   namespace detail
   {
      inline io::awaitable<void> sleep_for_or_stop(std::chrono::steady_clock::duration duration, std::stop_token token)
      {
         auto executor = co_await this_coro::executor;
         auto timer = io::steady_timer{executor};
         timer.expires_after(duration);
         auto stop_cb = cancel_on_stop(timer, token);
         auto [ec] = co_await timer.async_wait(ioe::as_tuple(io::use_awaitable));
         (void)ec;
      }

      inline auto notify_accept_error(server_event_handler *eh, tcp::endpoint const &endpoint, std::string message)
          -> void
      {
         if (eh == nullptr)
         {
            return;
         }
         eh->on_connection_event(server_event_handler::connection_event{
             .event_type = server_event_handler::connection_event::event::accept_error,
             .accept_endpoint = endpoint,
             .error_message = std::move(message)});
      }
   } // namespace detail

   io::awaitable<void> serve(app_context_base &context, server_config const &config, tcp::endpoint endpoint,
                             auto &&handler)
   {
      auto *event_handler = context.event_handler;
      auto executor = co_await this_coro::executor;

      // Persistent acceptor: opened once, reused for every async_accept. This avoids the bind/listen
      // overhead per connection and, more importantly, removes the per-call exception surface that
      // would otherwise terminate the accept loop on transient EMFILE / ENFILE.
      auto open_ec = error_code{};
      auto acceptor = try_open_acceptor(executor, endpoint, open_ec);
      if (open_ec)
      {
         detail::notify_accept_error(event_handler, endpoint,
                                     fmt::format("failed to open listening socket: {}", open_ec.message()));
         if (event_handler)
         {
            event_handler->on_connection_event(server_event_handler::connection_event{
                .event_type = server_event_handler::connection_event::event::listening_stopped,
                .accept_endpoint = endpoint,
                .error_message = open_ec.message()});
         }
         co_return;
      }

      if (event_handler)
      {
         event_handler->on_connection_event(server_event_handler::connection_event{
             .event_type = server_event_handler::connection_event::event::listening_started,
             .accept_endpoint = endpoint});
      }

      auto backoff = std::chrono::steady_clock::duration{kAcceptBackoffMin};
      std::optional<std::string> unexpected_exit_reason;

      while (!context.token.stop_requested())
      {
         std::optional<std::string> error_to_report;
         try
         {
            auto [ec, socket] = co_await accept_one(acceptor, context.token);
            if (context.token.stop_requested())
            {
               break;
            }
            if (!ec)
            {
               backoff = kAcceptBackoffMin;
               if (event_handler != nullptr)
               {
                  auto rep_ec = error_code{};
                  auto remote_ep = socket.remote_endpoint(rep_ec);
                  event_handler->on_connection_event(server_event_handler::connection_event{
                      .event_type = server_event_handler::connection_event::event::accepted_connection,
                      .accept_endpoint = endpoint,
                      .error_message = std::nullopt,
                      .client_endpoint = rep_ec ? std::nullopt : std::optional{remote_ep}});
               }

               // I must use socket's executor to execute handlers within the strand
               auto sock_executor = socket.get_executor();
               co_spawn(sock_executor,
                        handle_connection(context, config, std::move(socket),
                                          std::forward<std::decay_t<decltype(handler)>>(handler)),
                        io::detached);
               continue;
            }
            else
            {
               error_to_report = ec.message();
            }
         }
         catch (std::exception const &ex)
         {
            error_to_report = fmt::format("accept loop exception: {}", ex.what());
         }
         catch (...)
         {
            error_to_report = std::string{"accept loop exception: unknown"};
         }

         if (error_to_report.has_value())
         {
            detail::notify_accept_error(event_handler, endpoint, *error_to_report);
         }
         // Some errors (EMFILE/ENFILE) indicate FD pressure; back off briefly so we don't spin.
         co_await detail::sleep_for_or_stop(backoff, context.token);
         backoff = std::min<std::chrono::steady_clock::duration>(backoff * 2, kAcceptBackoffMax);
      }

      auto close_ec = error_code{};
      acceptor.close(close_ec);

      if (event_handler)
      {
         event_handler->on_connection_event(server_event_handler::connection_event{
             .event_type = server_event_handler::connection_event::event::listening_stopped,
             .accept_endpoint = endpoint,
             .error_message = unexpected_exit_reason});
      }
   }

} // namespace spider2
