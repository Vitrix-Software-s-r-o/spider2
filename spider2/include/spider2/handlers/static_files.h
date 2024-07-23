//
// Created by jhrub on 19.02.2023.
//

#pragma once

#include "../types.h"

namespace spider2
{

   struct static_files_config
   {
      cache_control caching = cache_control::make_cache_public();
      mime_table mime_types = {};
      vector<string> index_files = {};
      std::function<bool(request &, filesystem::path const &path)> file_path_filter;
   };

   inline auto static_files(filesystem::path folder, static_files_config config = {})
   {
      return [folder = std::move(folder), config = std::move(config)](request &req) -> await_response
      {
         auto ep = req.get_processing_endpoint();
         if (ep.verb == http::verb::head || ep.verb == http::verb::get)
         {

            if (std::string file_name;
                url::try_decode(file_name, ep.path.begin(), ep.path.end()) && file_name.find("..") == std::string::npos)
            {
               auto full_path = folder / file_name;
               if (filesystem::is_directory(full_path) && !config.index_files.empty())
               {
                  full_path = [&]
                  {
                     for (auto &index_file_name : config.index_files)
                     {
                        auto index_path = full_path / index_file_name;
                        if (filesystem::exists(index_path))
                        {
                           return index_path;
                        }
                     }
                     return full_path;
                  }();
               }

               if (!config.file_path_filter || config.file_path_filter(req, full_path))
               {
                  co_return return_file(
                      req, full_path,
                      file_response_settings{.mime_type = config.mime_types.lookup(full_path.extension().string()),
                                             .cache = config.caching});
               }
            }

            co_return response::return_string(http::status::not_found, "unable to find requested resource");
         }
         else
         {
            co_return response::return_string(http::status::method_not_allowed,
                                              "only GET and HEAD are allowed methods");
         }
      };
   }

} // namespace spider2