//
// Created by jhrub on 6/11/2023.
//

#pragma once
#include "../platform.h"
#include "request.h"
#include "response.h"

namespace spider2
{
   class cache_backend
   {
    public:
      virtual ~cache_backend() = default;

      virtual auto try_cache_result(string_view path, response const &resp) -> bool = 0;
      virtual auto try_get_cached_response(string_view path) const -> std::shared_ptr<http::response<http::string_body>> = 0;
   };

} // namespace spider2