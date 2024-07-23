//
// Created by jhrub on 3/7/2024.
//

#pragma once
#include "../types.h"

namespace spider2
{
    template <class T, class... Args>
    using awaitable_invoke_result_t = io::awaitable<std::invoke_result_t<T, Args...>>;

    /**
     * This should fix the issue with the capturing lambda for awaitable functions but it does not
     * @param fun_ - function that is going to be called
     * @return
     */
    template <class... Args>
    auto sync_handler(auto fun_)
    {
        using sync_handler_result_t = awaitable_invoke_result_t<decltype(fun_), Args...>;
        return [=](Args... args) -> sync_handler_result_t
        {
            return [](auto &fun, Args &&...args) -> sync_handler_result_t
            { co_return fun(std::forward<Args>(args)...); }(fun_, args...);
        };
    }
} // namespace spider2
