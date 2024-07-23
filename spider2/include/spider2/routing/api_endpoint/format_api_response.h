#pragma once

#include "../../types/structs.h"
#include "../../types/utils/tag_invoke.h"

namespace spider2
{
   /**
    * @brief Tag type for api_request customization point
    * Custom types must define a tag_invoke overload for this tag type
    *
    * @example
    *  struct custom_data_type
    * {
    *  private:
    *     friend auto tag_invoke(spider2::tag_t<spider2::format_api_response>, custom_data_type & value,
    * spider2::request & req) -> response
    *     {
    *       // custom response formatting logic
    *       return response::return_string(http::status::ok, "custom response");
    *     }
    *  };
    */
   inline constexpr struct format_api_response_fn
   {
      template <class T>
      auto operator()(request &req, T &&resp) const -> response
      {
         return tag_invoke(*this, req, std::forward<T>(resp));
      }

      friend auto tag_invoke(format_api_response_fn, request &req, response &&resp) -> response
      {
         return std::move(resp);
      }

   } format_api_response{};

} // namespace spider2