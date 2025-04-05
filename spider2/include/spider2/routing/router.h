//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include "../concepts/middleware.h"
#include "endpoint_handler.h"
#include "nested_router.h"
#include "scoped_router.h"

namespace spider2
{

   struct router_policy
   {
      struct break_on_not_found
      {
         template <class Req, class Handlers, class... Arg>
         await_response operator()(Handlers &handlers, Req &req, Arg &&...args) const
         {
            const endpoint_base ep = req.get_processing_endpoint();

            auto result = optional<response>{};
            auto exec = [&](auto &handler) -> io::awaitable<bool>
            {
               return [](auto &_handler, auto &_req, auto &_ep, auto &_result, auto &..._args) -> io::awaitable<bool>
               {
                  if (!_result.has_value() && _handler.matches(_ep))
                  {
                     //    std::cout << "handler matches : " << _ep.path << std::endl;
                     _result = co_await _handler.invoke(_req, std::forward<Arg>(_args)...);
                  }
                  // continue searching
                  co_return !_result.has_value() || !_result->is_handled_with_no_response();
               }(handler, req, ep, result, args...);
            };

            bool handled = co_await std::apply(
                [&](auto &...handler) -> io::awaitable<bool>
                {
                   return [](auto &exec_fun, auto &...handler_fun) -> io::awaitable<bool>
                   { co_return (... && co_await exec_fun(handler_fun)); }(exec, handler...);
                },
                handlers);

            static_cast<void>(handled);

            if (result.has_value())
               co_return std::move(result.value());
            else
               co_return response::return_string(http::status::not_found, "not found");
         }
      };

      struct continue_searching_on_not_found
      {
         template <class Req, class Handlers, class... Arg>
         await_response operator()(Req &req, Handlers &handlers, Arg &&...args) const
         {
            const endpoint_base ep = req.get_processing_endpoint();
            auto result = optional<response>{};
            auto exec = [&](auto &handler) -> io::awaitable<bool>
            {
               return [](auto &_req, auto &_result, auto &_ep, auto &_handler, Arg &&..._args) -> io::awaitable<bool>
               {
                  if (!_result.has_value() && _handler.matches(_ep))
                  {
                     // std::cout << "2. handler matches : " << _ep.path << std::endl;
                     _result = co_await _handler.invoke(_req, std::forward<Arg>(_args)...);

                     if (_result.has_value() && _result->get_status() == status::not_found)
                     {
                        _result.reset();
                     }
                  }

                  // continue searching
                  co_return !_result.has_value() || !_result->is_handled_with_no_response();
               }(req, result, ep, handler, args...);
            };

            bool handled = co_await std::apply(
                [&](auto &...handler) -> io::awaitable<bool>
                {
                   return [](auto &exec_fun, auto &...handler_fun) -> io::awaitable<bool>
                   { co_return (... && co_await exec_fun(handler_fun)); }(exec, handler...);
                },
                handlers);

            static_cast<void>(handled);

            if (result.has_value())
               co_return std::move(result.value());
            else
               co_return response::return_string(http::status::not_found, "not found");
         }
      };
   };

   template <class Policy, class... F>
   struct router
   {
      std::tuple<F...> handlers;
      Policy execution_policy{};

      template <class Next>
      auto operator+(Next &&next)
      {
         return router<Policy, F..., Next>{std::tuple_cat(handlers, std::tuple{std::forward<Next>(next)})};
      }

      template <class Next>
      auto on_path(string_view path, Next &&next)
      {
         using nested_next_type = nested_router<Next>;
         return router<Policy, F..., nested_next_type>{
             std::tuple_cat(handlers, std::tuple{nested_next_type{std::forward<Next>(next), path}})};
      }

      auto matches(endpoint_base const &ep) const noexcept -> bool
      {
         return std::apply([&](auto &...handler) -> bool { return (... || handler.matches(ep)); }, this->handlers);
      }

      template <class... Arg>
      auto invoke(request &req, Arg &&...args) const -> await_response
      {
         return this->execution_policy(this->handlers, req, std::forward<Arg>(args)...);
      }

      template <class... Arg>
      auto operator()(request &req, Arg &&...args) const -> await_response
      {
         return this->invoke(req, std::forward<Arg>(args)...);
      }
   };

   template <class Policy>
   struct router<Policy>
   {
      template <class Next>
      auto operator+(Next &&next)
      {
         return router<Policy, Next>{std::tuple{std::forward<Next>(next)}};
      }

      await_response operator()(auto &req) const
      {
         co_return response::return_string(status::not_implemented, "not implemented");
      }
   };

   template <class Policy = router_policy::break_on_not_found>
   auto begin_app()
   {
      return router<Policy>{};
   }
} // namespace spider2