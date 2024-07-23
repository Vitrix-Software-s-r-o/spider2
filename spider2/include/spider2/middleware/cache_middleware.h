//
// Created by jhrub on 6/1/2023.
//

#pragma once
#include "../types.h"
namespace spider2
{
   const auto cache_middleware = [](cache_backend &cache)
   {
      return [&](auto fun)
      {
         return [=, &cache](request &req, auto &&...ctx) -> await_response
         {
            return [](cache_backend &cache, auto const &fun, request &req, auto &&...ctx) -> await_response
            {
               auto can_be_cached = [](response const &r)
               {
                  if (r.get_status() == http::status::ok)
                  {
                     if (auto *headers = r.get_headers(); headers != nullptr)
                     {
                        auto etag_it = headers->find(http::field::etag);
                        return etag_it != headers->end();
                     }
                  }
                  return false;
               };
               auto endpoint = req.get_endpoint();
               if (endpoint.verb == verb::get)
               {
                  auto path_with_query = req.get_raw_path_include_query();
                  auto &request_headers = req.get_headers();
                  if (auto request_if_none_match_header = request_headers.find(http::field::if_none_match);
                      request_if_none_match_header != request_headers.end())
                  {
                     auto r = co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
                     if (can_be_cached(r))
                     {
                        cache.try_cache_result(path_with_query, r);
                     }
                     co_return r;
                  }
                  else if (auto cached_response_ptr = cache.try_get_cached_response(path_with_query);
                           cached_response_ptr != nullptr)
                  {
                     auto &cached_response = *cached_response_ptr;
                     if (auto cached_etag_header = cached_response.find(http::field::etag);
                         cached_etag_header != cached_response.end())
                     {
                        auto if_none_match_request = http::request<http::empty_body>{};
                        copy_headers_to_message(req.get_headers(), if_none_match_request);
                        if_none_match_request.method(http::verb::get);
                        if_none_match_request.set(http::field::if_none_match, cached_etag_header->value());
                        if_none_match_request.target({path_with_query.data(), path_with_query.size()});

                        auto wrapped = request{std::move(if_none_match_request), req.get_app_context()};
                        auto r = co_await fun(wrapped, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
                        switch (r.get_status())
                        {
                        case http::status::ok:
                           if (can_be_cached(r))
                           {
                              cache.try_cache_result(path_with_query, r);
                              co_return r;
                           }
                           break;
                        case http::status::not_modified:
                           co_return response{.message = {cached_response}};
                        default:
                           break;
                        }

                        co_return r;
                     }
                  }

                  auto r = co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
                  if (can_be_cached(r))
                  {
                     cache.try_cache_result(path_with_query, r);
                  }
                  co_return r;
               }
               else
               {
                  co_return (co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...));
               }

               // capture coroutine arguments correctly before its suspension
            }(cache, fun, req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
         };
      };
   };
} // namespace spider2