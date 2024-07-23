//
// Created by jhrub on 3/9/2024.
//

#pragma once
#include <string_view>

namespace spider2::api
{
   struct boost_json_formatter_tag
   {
      static constexpr std::string_view name = "boost_json";
   };
} // namespace spider2::api