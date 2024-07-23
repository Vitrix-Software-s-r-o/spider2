//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include "api_endpoint/api_endpoint.h"
#include "endpoint_handler.h"

namespace spider2
{

   template <class F>
   auto on_api_get(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::get, path, wrap_api_function(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_api_post(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::post, path, wrap_api_function(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_api_put(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::put, path, wrap_api_function(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_api_head(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::head, path, wrap_api_function(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_api_delete(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::delete_, path, wrap_api_function(std::forward<F>(_fun))};
   }

   template <class F>
   auto on_api_options(string_view path, F &&_fun)
   {
      return endpoint_handler{http::verb::options, path, wrap_api_function(std::forward<F>(_fun))};
   }
} // namespace spider2