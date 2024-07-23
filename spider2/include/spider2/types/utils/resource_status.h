//
// Created by jhrub on 04.03.2023.
//

#pragma once
#include "../platform.h"
namespace spider2
{
   enum class resource_status
   {
      /// it indicates there is not required header (if_modified_since or if_none_match)
      fresh,
      /// it indicates that resource has not been modified and client should use the cached value
      not_modified,
      /// it indicates that there is going to be new response body
      modified
   };

   inline static auto get_modified_since_status(http::fields const &headers,
                                                chrono::system_clock::time_point modification_date) noexcept
       -> resource_status
   {
      auto value = headers.find(http::field::if_modified_since);
      if (value != headers.end())
      {
         auto value_view = value->value();
         chrono::system_clock::time_point if_modified_since;
         return try_parse_http_date(if_modified_since, {value_view.data(), value_view.size()}) &&
                        if_modified_since < modification_date
                    ? resource_status::modified
                    : resource_status::not_modified;
         ;
      }
      return resource_status::fresh;
   }

   inline static auto get_etag_changed_status(http::fields const &headers, string_view etag) noexcept -> resource_status
   {
      auto value = headers.find(http::field::if_none_match);
      if (value != headers.end())
      {
         auto value_view = value->value();

         return string_view{value_view.data(), value_view.size()}.find(etag) == string_view::npos
                    ? resource_status::modified
                    : resource_status::not_modified;
      }
      return resource_status::fresh;
   }

   inline static auto get_resource_status(http::fields const &headers, string_view etag,
                                          chrono::system_clock::time_point modification_date) noexcept
       -> resource_status
   {
      auto etag_status = get_etag_changed_status(headers, etag);
      if (etag_status == resource_status::modified || etag_status == resource_status::not_modified)
      {
         return etag_status;
      }
      return get_modified_since_status(headers, modification_date);
   }
} // namespace spider2