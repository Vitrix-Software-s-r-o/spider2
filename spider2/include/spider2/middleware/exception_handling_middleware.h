//
// Created by jhrub on 25.12.2022.
//

#pragma once

#include "../types.h"

#include "make_async_execution_handler.h"

namespace spider2
{
   template <class ExecutorT, class MiddlewareT>
   class exception_handling_middleware
   {
    public:
      exception_handling_middleware(ExecutorT &executor, MiddlewareT next) : executor_(executor), next_(std::move(next))
      {
      }

      [[nodiscard]] auto operator()(request &req) noexcept -> io::awaitable<response>
      {
         return io::async_initiate<decltype(io::use_awaitable), void(response)>(
             [&](auto &&handler)
             {
                io::co_spawn(
                    executor_,
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
      ExecutorT &executor_;
      MiddlewareT next_;
   };

} // namespace spider2