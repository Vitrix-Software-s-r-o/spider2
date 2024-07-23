//
// Created by jhrub on 6/11/2023.
//

#pragma once
#include "cache_backend.h"

namespace spider2
{
   class memory_cache_backend : public cache_backend
   {
    public:
      memory_cache_backend(std::size_t limit);

      auto try_cache_result(string_view path, response const &resp) -> bool final;
      auto try_get_cached_response(string_view path) const -> std::shared_ptr<http::response<http::string_body>> final;

      auto fit_to_limits() -> void;

    private:
      using cache_map = unordered_map<string, std::shared_ptr<http::response<http::string_body>>>;
      struct metadata
      {
         std::int64_t content_length;
         std::chrono::steady_clock::time_point last_access_time;
      };
      using metadata_map = unordered_map<string, metadata>;

      mutable std::mutex lock_;
      cache_map cache_;
      mutable metadata_map metadata_;
      std::int64_t total_content_length_ = 0;
      std::int64_t memory_limit_ = 0;
      std::size_t cache_version_ = 0;
   };

} // namespace spider2