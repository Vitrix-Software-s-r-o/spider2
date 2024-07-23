//
// Created by jhrub on 25.12.2022.
//

#pragma once

#include "../types.h"
#include <exception>

namespace spider2
{

   auto make_async_execution_handler(auto &&handler)
   {
      const auto handle_exception = [](auto &handler, std::exception_ptr ex)
      {
         if (ex != nullptr)
         {
            try
            {
               std::rethrow_exception(ex);
            }
            catch (std::exception const &ex)
            {
               // LOG(ERROR) << "unexpected exception handled: " << ex.what();
               handler(response::return_string(http::status::internal_server_error,
                                               fmt::format("<h1>Internal server error</h1><p>{}</p>", ex.what())));
            }
            catch (...)
            {
               // LOG(ERROR) << "unknown exception handled";
               handler(response::return_string(http::status::internal_server_error,
                                               "<h1>Internal server error</h1><p>unknown</p>"));
            }
            return true;
         }
         return false;
      };

      return [handler = std::forward<std::decay_t<decltype(handler)>>(handler),
              handle_exception = handle_exception](std::exception_ptr ex, auto &&res) mutable
      {
         if (!handle_exception(handler, ex))
         {
            handler(std::forward<std::decay_t<decltype(res)>>(res));
         }
      };
   }

} // namespace spider2