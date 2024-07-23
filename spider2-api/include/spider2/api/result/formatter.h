//
// Created by jhrub on 3/9/2024.
//

#pragma once

#include "is_api_result.h"
#include <spider2/types.h>

namespace spider2::api
{

   inline constexpr struct format_result_value_fn
   {
      template <class Tag, api_result_c T>
      auto operator()(Tag &&tag, request &req, T const &resp) const -> string
      {
         return tag_invoke(*this, std::forward<Tag>(tag), req, resp);
      }
   } format_result_value{};

   inline constexpr struct get_result_value_content_type_fn
   {
      template <class Tag, api_result_c T>

      auto operator()(Tag &&tag, request &req, T const &resp) const -> string
      {
         return tag_invoke(*this, std::forward<Tag>(tag), req, resp);
      }

      friend auto tag_invoke(get_result_value_content_type_fn, default_formatter_tag, request &,
                             api_result_c auto const &resp) -> string
      {
         return "application/json";
      }
   } get_result_value_content_type{};

} // namespace spider2::api
