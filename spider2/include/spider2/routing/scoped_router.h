//
// Created by jhrub on 12.02.2023.
//

#pragma once

#include "../types.h"

namespace spider2 {

    template<class R>
    struct scoped_router {
        R router;
        string_view path;

        auto matches(endpoint_base const &base) const noexcept -> bool;

        template<class... Arg>
        auto invoke(request &req, Arg &&...args) const noexcept -> await_response {
            auto offset_handle = req.push_path_processing_offset(static_cast<std::uint32_t>(path.size()));
            co_return (co_await router(req, std::forward<Arg>(args)...));
        }
    };

    template<class R>
    auto scoped_router<R>::matches(endpoint_base const &base) const noexcept -> bool {
        return base.path.starts_with(path);
    }

    template<class F, class Router>
    auto on_scope(string_view path, F &&middleware, Router &&router) {
        return scoped_router{middleware(std::forward<Router>(router)), path};
    }

    template<class Router>
    auto on_scope(string_view path, Router &&router) {
        return scoped_router{std::forward<Router>(router), path};
    }
}