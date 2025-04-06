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
   inline io::awaitable<std::tuple<boost::system::error_code, tcp::socket>> accept_tcp(std::stop_token token,
                                                                                       tcp::endpoint endpoint)
   {
      auto executor = co_await this_coro::executor;

      auto acceptor = tcp::acceptor{executor, endpoint};
      auto watchdog = cancel_on_stop(acceptor, token);

      auto [ec, socket] = co_await acceptor.async_accept(io::make_strand(executor), ioe::as_tuple(io::use_awaitable));

      co_return std::tuple{ec, std::move(socket)};
   }
} // namespace spider2