//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include "endpoint_handler.h"
#include "../concepts/middleware.h"
#include "nested_router.h"
#include "scoped_router.h"

namespace spider2 {

    struct router_policy {
        struct break_on_not_found {
            template<class Req, class Handlers, class...Arg>
            await_response operator()(Handlers &handlers, Req &req, Arg &&...args) const {
                const endpoint_base ep = req.get_processing_endpoint();

                auto result = optional<response>{};
                auto exec = [&](auto &handler) -> io::awaitable<bool> {
                    if (handler.matches(ep)) {
                        result = co_await handler.invoke(req, std::forward<Arg>(args)...);

                        //it causes next executor to not run
                        co_return false;
                    }

                    // continue searching
                    co_return true;
                };

                co_await std::apply([&](auto &...handler) -> io::awaitable<void> {
                    (... && co_await exec(handler));
                }, handlers);

                if (result.has_value())
                    co_return std::move(result.value());
                else
                    co_return response::return_string(http::status::not_found, "not found");
            }
        };

        struct continue_searching_on_not_found {
            template<class Req, class Handlers, class...Arg>
            await_response operator()(Req &req, Handlers &handlers, Arg &&... args) const {
                const endpoint_base ep = req.get_processing_endpoint();
                auto result = optional<response>{};
                auto exec = [&](auto &handler) -> io::awaitable<bool> {
                    if (handler.matches(ep)) {
                        result = co_await handler.invoke(req, std::forward<Arg>(args)...);
                        co_return result->get_status() == status::not_found;
                    }

                    // continue searching
                    co_return true;
                };

                bool handled = co_await std::apply([&](auto &...handler) -> io::awaitable<bool> {
                    co_return (... && co_await exec(handler));
                }, handlers);

                if (result.has_value())
                    co_return std::move(result.value());
                else
                    co_return response::return_string(http::status::not_found, "not found");
            }
        };

    };


    template<class Policy, class... F>
    struct router {
        std::tuple<F...> handlers;
        Policy execution_policy{};

        template<class Next>
        auto operator+(Next &&next) {
            return router<Policy, F..., Next>{std::tuple_cat(handlers, std::tuple{std::forward<Next>(next)})};
        }

        template<class Next>
        auto on_path(string_view path, Next &&next) {
            using nested_next_type = nested_router<Next>;
            return router<Policy, F..., nested_next_type>{
                    std::tuple_cat(handlers, std::tuple{nested_next_type{std::forward<Next>(next), path}})};
        }

        auto matches(endpoint_base const &ep) const noexcept -> bool {
            return std::apply([&](auto &...handler) -> bool {
                return (... || handler.matches(ep));
            }, this->handlers);
        }

        template<class... Arg>
        auto invoke(request &req, Arg &&...args) const -> await_response {
            return this->execution_policy(this->handlers, req, std::forward<Arg>(args)...);
        }

        template<class... Arg>
        auto operator()(request &req, Arg &&...args) const -> await_response {
            return this->invoke(req, std::forward<Arg>(args)...);
        }
    };

    template<class Policy>
    struct router<Policy> {
        template<class Next>
        auto operator+(Next &&next) {
            return router<Policy, Next>{std::tuple{std::forward<Next>(next)}};
        }

        await_response operator()(auto &req) const {
            co_return response::return_string(status::not_implemented, "not implemented");
        }
    };

    template<class Policy = router_policy::break_on_not_found>
    auto begin_app() {
        return router<Policy>{};
    }
}