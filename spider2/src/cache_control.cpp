#include "spider2/types/structs/cache_control.h"
#include <fmt/format.h>

spider2::cache_control spider2::cache_control::make_immutable(int max_age)
{
   cache_control response;
   response.cache_type = immutable_resource;
   response.max_age = max_age;
   return response;
}

spider2::cache_control spider2::cache_control::make_cache_public(int max_age)
{
   cache_control response;
   response.cache_type = cache_public;
   response.max_age = max_age;
   return response;
}

spider2::cache_control spider2::cache_control::make_cache_private(int max_age)
{
   cache_control response;
   response.cache_type = cache_private;
   response.max_age = max_age;
   return response;
}

spider2::cache_control spider2::cache_control::make_no_cache()
{
   cache_control response;
   response.cache_type = no_cache;

   return response;
}

auto spider2::cache_control::set_headers(http::fields &resp) const -> void
{
   if (cache_type == no_cache)
   {
      resp.set(http::field::cache_control, "no-cache, no-store, must-revalidate");
   }
   else
   {
      std::string value;
      if ((cache_type & immutable_resource) != 0)
      {
         if ((cache_type & cache_private) != 0)
            value = fmt::format("private, max-age={}, immutable", max_age);
         else
         {
            value = fmt::format("max-age={}, immutable", max_age);
         }
      }
      else if ((cache_type & cache_private) != 0)
      {
         value = fmt::format("private, max-age={}", max_age);
      }
      else
      {
         value = fmt::format("public, max-age={}", max_age);
      }

      resp.set(http::field::cache_control, value);
   }
}