#pragma once
#include "../types.h"
namespace spider2
{
   const auto cors_middleware = [](string_view access_control_allow_origin = "*")
   {
      return [=](auto fun)
      {
         return [=](request &req, auto &&...ctx) -> await_response
         {
            return [](auto const &fun, request &req, string_view access_control_allow_origin, auto &&...ctx) -> await_response
            {
               auto resp = co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
               resp.try_set_header(http::field::access_control_allow_origin, access_control_allow_origin);

               co_return resp;
            // capture coroutine arguments before suspension
            }(fun, req, access_control_allow_origin, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
         };
      };
   };
}