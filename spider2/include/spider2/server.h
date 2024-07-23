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

   io::awaitable<void> handle_connection(app_context_base &context, server_config const &config, tcp::socket socket,
                                         auto &&handler)
   {
      // std::cout << "start" << std::endl;;
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
                   explicit local_handler(tcp::socket &&socket) : socket_(std::move(socket))
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
                // We send here 408 - to indicate connection close
                // https://developer.mozilla.org/en-US/docs/Web/HTTP/Status/408
                http::async_write(handler_ptr->socket_, handler_ptr->message_,
                                  [handler_ptr](error_code const &, size_t) mutable { handler_ptr->finish_close(); });
             },
             config.connection_keep_alive);

         auto ec = co_await request_obj.read_header();
         keep_alive_watchdog.cancel();

         if (ec)
         {
            break;
         }

         auto response_message = co_await handler(request_obj);

         if (!socket.is_open())
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
   }

   io::awaitable<void> serve(app_context_base &context, server_config const &config, tcp::endpoint endpoint,
                             auto &&handler)
   {
      while (!context.token.stop_requested())
      {
         auto [ec, socket] = co_await accept_tcp(context.token, endpoint);
         if (!ec)
         {
            // I must use socket's executor to execute handlers within the strand
            auto executor = socket.get_executor();
            co_spawn(executor,
                     handle_connection(context, config, std::move(socket),
                                       std::forward<std::decay_t<decltype(handler)>>(handler)),
                     io::detached);
         }
      }
   }

} // namespace spider2
