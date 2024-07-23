//
// Created by jhrub on 27.01.2023.
//

#pragma once

#include "../types.h"

namespace spider2 {

    template<class R>
    struct nested_router {
        R router;
        string_view path;

        auto matches(endpoint_base const &base) const noexcept -> bool;

        template<class... Arg>
        auto invoke(request &req, Arg &&...args) const noexcept -> await_response {
            auto offset_handle = req.push_path_processing_offset(static_cast<std::uint32_t>(path.size()));
            co_return (co_await router.invoke(req, std::forward<Arg>(args)...));
        }
    };


    template<class R>
    auto nested_router<R>::matches(endpoint_base const &base) const noexcept -> bool {
        if (base.path.starts_with(path)) {
            auto nested_ep = base;
            nested_ep.path = nested_ep.path.substr(path.size());
            return router.matches(nested_ep);
        }
        return false;
    }


}