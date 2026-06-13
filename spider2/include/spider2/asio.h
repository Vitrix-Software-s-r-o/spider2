#pragma once

#include "types.h"

namespace spider2
{

   template <class T>
   auto cancel_on_stop(T &cancellable, stop_token token)
   {
      return make_stop_callback(token, [&]() { cancellable.cancel(); });
   }

   template <class Executor, class T>
   auto cancel_after_timeout(Executor &context, T cancel_fun, chrono::steady_clock::duration duration)
       -> io::steady_timer
   {
      auto timer = io::steady_timer{context};
      timer.expires_after(duration);
      timer.async_wait(
          [=](boost::system::error_code const &ec)
          {
             if (!ec)
             {
                cancel_fun();
             }
          });

      return timer;
   }

   template <class Executor, class Fun>
   struct timer_stop_callback_base
   {
      Fun cancel_fun;
      io::steady_timer timer;

      explicit timer_stop_callback_base(Executor &ex, Fun &&cancel_fun)
          : cancel_fun{std::forward<Fun>(cancel_fun)}, timer{ex}
      {
      }

      static auto make_stop_token_cancellation(timer_stop_callback_base *self)
      {
         return [self]
         {
            // just to silence the warning
            if (self == nullptr)
            {
               return;
            }

            self->timer.cancel();
            self->cancel_fun();
         };
      }
   };

   template <class Executor, class Fun>
   struct timer_stop_callback : public timer_stop_callback_base<Executor, Fun>
   {
      using callback_type = decltype(timer_stop_callback_base<Executor, Fun>::make_stop_token_cancellation(nullptr));

      stop_callback<callback_type> stop_cb;

      explicit timer_stop_callback(Executor &executor, Fun &&cancel_fun, chrono::steady_clock::duration duration,
                                   std::stop_token token)
          : timer_stop_callback_base<Executor, Fun>(executor, std::forward<Fun>(cancel_fun)),
            stop_cb{token, timer_stop_callback_base<Executor, Fun>::make_stop_token_cancellation(this)}
      {
         this->timer.expires_after(duration);
         this->timer.async_wait(
             [this](boost::system::error_code const &ec)
             {
                if (!ec)
                {
                   this->cancel_fun();
                }
             });
      }
   };

   template <class Executor, class T>
   auto cancel_after_timeout_or_token(Executor &context, stop_token token, T &&cancel_fun,
                                      chrono::steady_clock::duration duration)
   {
      return timer_stop_callback{context, std::forward<T>(cancel_fun), duration, token};
   }
   /// Open and configure a listening acceptor without throwing.
   /// Returns a default-constructed acceptor and a populated error_code on failure.
   inline auto try_open_acceptor(auto &executor, tcp::endpoint const &endpoint, boost::system::error_code &ec)
       -> tcp::acceptor
   {
      auto acceptor = tcp::acceptor{executor};
      acceptor.open(endpoint.protocol(), ec);
      if (ec)
      {
         return acceptor;
      }
      acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
      if (ec)
      {
         return acceptor;
      }
      acceptor.bind(endpoint, ec);
      if (ec)
      {
         return acceptor;
      }
      acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
      return acceptor;
   }

   /// Accept a single connection on an already-opened acceptor.
   /// Cancels on stop; returns the asio error_code and resulting socket.
   inline io::awaitable<std::tuple<boost::system::error_code, tcp::socket>> accept_one(tcp::acceptor &acceptor,
                                                                                       std::stop_token token)
   {
      auto executor = co_await this_coro::executor;
      auto watchdog = cancel_on_stop(acceptor, token);

      auto [ec, socket] = co_await acceptor.async_accept(io::make_strand(executor), ioe::as_tuple(io::use_awaitable));

      co_return std::tuple{ec, std::move(socket)};
   }

   /// Legacy one-shot accept that opens a new acceptor each call.
   /// Prefer accept_one() with a long-lived accaccept_tcpeptor in production code.
   inline io::awaitable<std::tuple<boost::system::error_code, tcp::socket>> accept_tcp(std::stop_token token,
                                                                                       tcp::endpoint endpoint)
   {
      auto executor = co_await this_coro::executor;

      auto open_ec = boost::system::error_code{};
      auto acceptor = try_open_acceptor(executor, endpoint, open_ec);
      if (open_ec)
      {
         co_return std::tuple{open_ec, tcp::socket{executor}};
      }

      auto [ec, socket] = co_await accept_one(acceptor, token);
      co_return std::tuple{ec, std::move(socket)};
   }
} // namespace spider2