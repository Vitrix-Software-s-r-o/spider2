//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include <concepts>
#include "spider2/types/structs/endpoint.h"
#include "spider2/types/structs/request.h"
#include "spider2/types/structs/response.h"

namespace spider2::concepts {

    template<class T, class ... Args>
    concept sync_middleware = requires(T fun, request &req, Args&& ... args)
    {
        { fun(req, std::forward<Args>(args)...) } -> std::same_as<response>;
    };
}