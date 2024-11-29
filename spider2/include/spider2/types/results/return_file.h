//
// Created by jhrub on 04.03.2023.
//

#pragma once

#include "spider2/types/structs/request.h"
#include "spider2/types/structs/response.h"

#include "spider2/types/structs/cache_control.h"
#include "spider2/types/utils/etag.h"
#include "spider2/types/utils/http_date.h"
#include "spider2/types/utils/resource_status.h"

#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/random.hpp>

namespace spider2
{
   struct file_response_settings
   {
      string_view mime_type;
      cache_control cache;
      optional<http::fields> headers = {};
   };

   inline auto generate_boundary(chrono::system_clock::time_point time_point, std::uint64_t file_size) -> string
   {
      boost::mt19937 mt{static_cast<std::uint32_t>(time_point.time_since_epoch().count()) ^
                        static_cast<std::uint32_t>(file_size)};
      boost::uuids::basic_random_generator<boost::mt19937> gen{mt};
      return boost::lexical_cast<string>(gen());
   }

   inline auto return_file(request &req, filesystem::path const &file_path, file_response_settings const &setting = {})
      -> response
   {

      if (auto file = async_file_io::open_binary_file(req.get_executor(), file_path); file.has_value())
      {
         auto last_write_time = async_file_io::get_last_modified_time(file.value());
         auto file_size = async_file_io::get_file_size(file.value());

         if (!last_write_time.has_value() || file_size == std::numeric_limits<std::uint64_t>::max())
         {
            return response::return_string(http::status::internal_server_error, "unable to obtain requested file info");
         }

         auto etag_value = etag::make_etag(last_write_time.value(), file_size);
         auto status = get_resource_status(req.get_headers(), etag_value, last_write_time.value());

         const auto set_response_headers = [&](auto &resp)
         {
            resp.set(http::field::content_type, {setting.mime_type.data(), setting.mime_type.size()});
            resp.set(http::field::etag, etag_value);
            resp.set(http::field::accept_ranges, "bytes");
            resp.set(http::field::last_modified, format_http_date(last_write_time.value()));
            setting.cache.set_headers(resp);

            if (setting.headers.has_value())
            {
               for (auto &field : setting.headers.value())
               {
                  if (auto name = field.name(); name != http::field::unknown)
                  {
                     resp.set(name, field.value());
                  }
                  else
                  {
                     resp.set(field.name_string(), field.value());
                  }
               }
            }
         };

         auto &headers = req.get_headers();
         if (auto range_header = headers.find(http::field::range); range_header != headers.end())
         {
            auto range_header_value = range_header->value();
            auto byte_ranges_values = parse_byte_ranges({range_header_value.data(), range_header_value.size()});
            if (byte_ranges_values.has_value() && !byte_ranges_values->empty() &&
                validate_byte_ranges(file_size, byte_ranges_values.value()))
            {
               auto resp = file_byte_ranges_response{{},
                                                     std::move(file.value()),
                                                     {.ranges = std::move(byte_ranges_values.value()),
                                                      .boundary = generate_boundary(last_write_time.value(), file_size),
                                                      .file_size = file_size,
                                                      .write_body = req.get_endpoint().verb != http::verb::head &&
                                                                    status != resource_status::not_modified},
               };
               set_response_headers(resp);
               resp.result(http::status::partial_content);

               return response{std::move(resp)};
            }
            else
            {
               return response::return_string(http::status::range_not_satisfiable,
                                              "unable to satisfy the requested range");
            }
         }
         else
         {

            auto resp = file_response{{},
                                      std::move(file.value()),
                                      {.file_size = file_size,
                                       .write_body = req.get_endpoint().verb != http::verb::head &&
                                                     status != resource_status::not_modified},
            };
            set_response_headers(resp);
            resp.result(status == resource_status::not_modified ? http::status::not_modified : http::status::ok);

            return response{std::move(resp)};
         }
      }
      else
      {
         return response::return_string(http::status::not_found, "the requested file has not been found");
      }
   }
} // namespace spider2