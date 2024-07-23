//
// Created by jhrub on 25.12.2022.
//

#pragma once

#include "../types.h"

#include "make_async_execution_handler.h"

namespace spider2
{
   template <class MiddlewareT>
   class thread_pool_middleware
   {
    public:
      thread_pool_middleware(thread_pool &pool, MiddlewareT next) : pool_(pool), next_(std::move(next)) {}

      [[nodiscard]] auto operator()(request &req) noexcept -> io::awaitable<response>
      {
         return io::async_initiate<decltype(io::use_awaitable), void(response)>(
             [&](auto &&handler)
             {
                io::co_spawn(
                    pool_,
                    [&]() -> io::awaitable<response>
                    {
                       return [](auto const &next_, request &req) -> io::awaitable<response>
                       { co_return (co_await next_(req)); }(next_, req);
                    },
                    make_async_execution_handler(std::move(handler)));

                // handler(response::return_string(http::status::ok, "test"));
             },
             io::use_awaitable);
      }

    private:
      thread_pool &pool_;
      MiddlewareT next_;
   };

} // namespace spider2