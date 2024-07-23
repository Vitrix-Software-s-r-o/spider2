//
// Created by jhrub on 3/9/2024.
//

#pragma once
#ifdef SPIDER2_API_USE_VITRIX_COMMONS
#include <string_view>

namespace spider2::api
{
   struct commons_serialization_formatter_tag
   {
      static constexpr std::string_view name = "commons_serialization";
   };
} // namespace spider2::api

#endif