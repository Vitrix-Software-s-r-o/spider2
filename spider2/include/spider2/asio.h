#pragma once

#include "types.h"

namespace spider2 {

    template<class T>
    auto cancel_on_stop(T &cancellable, stop_token token) {
        return make_stop_callback(token, [&]() {
            cancellable.cancel();
        });
    }

    template<class Executor, class T>
    auto cancel_after_timeout(Executor &context, T cancel_fun,
                              chrono::steady_clock::duration duration) -> io::steady_timer {
        auto timer = io::steady_timer{context};
        timer.expires_from_now(duration);
        timer.async_wait([=](boost::system::error_code const &ec) {
            if (!ec) {
                cancel_fun();
            }
        });

        return timer;
    }

    inline io::awaitable<std::tuple<boost::system::error_code, tcp::socket>>
    accept_tcp(std::stop_token token, tcp::endpoint endpoint) {
        auto executor = co_await this_coro::executor;

        auto acceptor = tcp::acceptor{executor, endpoint};
        auto watchdog = cancel_on_stop(acceptor, token);

        auto [ec, socket] = co_await acceptor.async_accept(io::make_strand(executor), ioe::as_tuple(io::use_awaitable));

        co_return std::tuple{ec, std::move(socket)};
    }
}