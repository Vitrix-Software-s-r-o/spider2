#pragma once

#include "../platform.h"
namespace spider2
{
   struct cache_control
   {
      enum type
      {
         no_cache,
         immutable_resource = 0x02,

         cache_public = 0x04,
         cache_private = 0x08,

      };

      type cache_type = no_cache;
      int max_age = 0;

      static cache_control make_immutable(int max_age = 3600 * 24 * 365);

      static cache_control make_cache_public(int max_age = 3600);

      static cache_control make_cache_private(int max_age = 3600);

      static cache_control make_no_cache();

      auto set_headers(http::fields &resp) const -> void;
   };

} // namespace spider2
