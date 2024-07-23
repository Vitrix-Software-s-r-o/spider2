//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include "../types.h"

namespace spider2
{
   template <class F>
   struct endpoint_handler : public endpoint
   {
      F invoke;

      endpoint_handler(http::verb _verb, string_view _path, F &&_fun)
          : endpoint{_verb, _path}, invoke(std::forward<F>(_fun))
      {
      }
   };

   template <class F>
   auto on_get(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::get, path, wrap_any_function_async(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_post(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::post, path, wrap_any_function_async(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_put(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::put, path, wrap_any_function_async(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_head(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::head, path, wrap_any_function_async(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_delete(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::delete_, path, wrap_any_function_async(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_options(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::options, path, wrap_any_function_async(std::forward<F>(_fun))};
   }
} // namespace spider2