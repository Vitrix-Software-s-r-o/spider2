//
// Created by jhrub on 6/11/2023.
//
#include "spider2/types/structs/memory_cache_backend.h"
#include <gsl/narrow>

spider2::memory_cache_backend::memory_cache_backend(std::size_t limit) : memory_limit_(gsl::narrow<std::int64_t>(limit))
{
}
auto spider2::memory_cache_backend::try_cache_result(string_view path, const response &resp) -> bool
{
   if (const auto *message_ptr = resp.try_get_message<http::string_body>(); message_ptr != nullptr)
   {
      auto key = static_cast<string>(path);
      auto body_size = gsl::narrow<std::int64_t>(message_ptr->body().size());
      auto cache_resp_ptr = std::make_shared<http::response<http::string_body>>(*message_ptr);

      auto lk = std::lock_guard{lock_};

      if (auto it = cache_.find(key); it != cache_.end())
      {
         it->second = std::move(cache_resp_ptr);

         auto &m = metadata_[key];
         total_content_length_ -= m.content_length;
         total_content_length_ += body_size;
         m.content_length = body_size;
         m.last_access_time = std::chrono::steady_clock::now();
      }
      else
      {
         cache_[key] = std::move(cache_resp_ptr);
         auto &m = metadata_[key];
         m.content_length = body_size;
         m.last_access_time = std::chrono::steady_clock::now();
      }

      total_content_length_ += body_size;
      ++cache_version_;
      return true;
   }
   return false;
}
auto spider2::memory_cache_backend::try_get_cached_response(string_view path) const -> std::shared_ptr<http::response<http::string_body>>
{
   auto key = static_cast<string>(path);

   auto lk = std::lock_guard{lock_};
   if (auto it = cache_.find(key); it != cache_.end())
   {
      auto &m = metadata_[key];
      m.last_access_time = std::chrono::steady_clock::now();

      return it->second;
   }

   return nullptr;
}
auto spider2::memory_cache_backend::fit_to_limits() -> void
{
   // optimistic locking func, I break when I am able to commit changes
   while (true)
   {
      auto clear_cache_need = [&]() -> std::optional<std::tuple<cache_map, metadata_map, std::int64_t, std::size_t>>
      {
         auto lk = std::lock_guard{lock_};
         if (total_content_length_ > memory_limit_)
         {
            return std::make_tuple(cache_, metadata_, total_content_length_, cache_version_);
         }
         else
         {
            return {};
         }
      }();

      if (clear_cache_need.has_value())
      {
         auto &[cache, cache_metadata, total_len, cache_version] = clear_cache_need.value();
         auto sorted_metadata = [&]()
         {
            using kv_t = std::pair<string, metadata>;
            auto result = std::vector<kv_t>{};
            result.reserve(cache_metadata.size());
            result.insert(result.begin(), cache_metadata.begin(), cache_metadata.end());
            std::sort(result.begin(), result.end(),
                      [](kv_t const &a, kv_t const &b)
                      { return a.second.last_access_time < b.second.last_access_time; });

            return result;
         }();

         for (auto &kv : sorted_metadata)
         {
            cache.erase(kv.first);
            cache_metadata.erase(kv.first);
            total_len -= kv.second.content_length;
            if (total_len < memory_limit_)
            {
               break;
            }
         }

         auto lock = std::lock_guard{lock_};
         if (cache_version == cache_version_)
         {
            cache_ = std::move(cache);
            metadata_ = std::move(cache_metadata);
            total_content_length_ = total_len;
            cache_version_ = 0;
         }
      }
      else
      {
         break;
      }
   }
}
