//
// Created by jhrub on 3/9/2024.
//

#pragma once
#include "flat_value.h"
#include <spider2/types.h>
namespace spider2::api
{
   enum class op_msg_severity
   {
      error,
      warning,
      info

   };

   inline void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, op_msg_severity const &msg)
   {
      switch (msg)
      {
      case op_msg_severity::error:
         jv = "error";
         break;
      case op_msg_severity::warning:
         jv = "warning";
         break;
      case op_msg_severity::info:
         jv = "info";
         break;
      default:
         jv = "unknown";
         break;
      }
   }
   /**
    * @brief This struct is used to represent a message that is sent to the user.
    */
   struct op_msg
   {
      /**
       * @brief The message that is sent to the user.
       * We use a T: prefixed string to allow for easy translation of the message on the client side
       */
      string message;

      /**
       * @brief The tokens that are used to fill in the message.
       */
      vector<flat_value> tokens;

      /**
       * @brief The severity of the message.
       */
      optional<op_msg_severity> severity;

      friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, op_msg const &msg)
      {
         jv = {{"message", boost::json::string{msg.message}}, {"tokens", boost::json::value_from( msg.tokens)}};

         if (msg.severity.has_value())
         {
            jv.as_object()["severity"] = boost::json::value_from(msg.severity.value());
         }
      }
   };
} // namespace spider2::api