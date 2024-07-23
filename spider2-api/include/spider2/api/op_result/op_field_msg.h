//
// Created by jhrub on 11.12.2022.
//

#pragma once

#include "flat_value.h"
#include "op_msg.h"
#include <spider2/types.h>

namespace spider2::api
{
   struct op_field_msg : op_msg
   {
      /**
       * @brief The field_id that is used to identify the field that is being referred to.
       */
      string field_id;

      friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, op_field_msg const &msg)
      {
         jv = boost::json::value_from(static_cast<op_msg const &>(msg));
         jv.as_object()["field_id"] = msg.field_id;
      }
   };
} // namespace spider2::api