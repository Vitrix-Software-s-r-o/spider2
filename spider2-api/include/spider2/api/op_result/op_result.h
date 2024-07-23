//
// Created by jhrub on 11.12.2022.
//

#pragma once

#include "../result.h"
#include "op_field_msg.h"

namespace spider2::api
{
   /**
    * @brief A generic operation result that is used to return the result of an operation.
    * We use this to standardize the way we return the result of an operation.
    *
    * It cover most of the use cases that we have in our operations.
    *
    * @tparam T - The type of the value that is returned by the operation.
    */
   template <class T = flat_value>
   struct op_result
   {
      using self_t = op_result<T>;
      optional<T> value;
      optional<op_msg> message;
      optional<vector<op_field_msg>> field_messages;

      inline static result<self_t> make_success_result(optional<T> value = {}, op_msg message = {"T:operation.success"})
      {
         return {self_t{std::move(value), {std::move(message)}}, status::ok};
      }

      inline static result<self_t> make_validation_failed_result(vector<op_field_msg> validation_errors,
                                                                 op_msg message = {"T:operation.validation.failed"})
      {
         return {{{}, {std::move(message)}, std::move(validation_errors)}, status::expectation_failed};
      }

      inline static result<self_t> make_entity_not_found_result(optional<T> value,
                                                                op_msg message = {"T:entity.not.found"})
      {
         return {{std::move(value), {std::move(message)}}, status::not_found};
      }

      inline static result<self_t> make_failed_result(op_msg message = {"T:operation.failed"})
      {
         return {{{}, {std::move(message)}}, status::bad_request};
      }

      inline static result<self_t> make_forbidden_result(op_msg message = {"T:operation.forbidden"})
      {
         return {{{}, {std::move(message)}}, status::forbidden};
      }

      friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, self_t const &msg)
      {
         auto &obj = jv.emplace_object();
         if (msg.value.has_value())
         {
            obj["value"] = boost::json::value_from(msg.value.value());
         }
         if (msg.message.has_value())
         {
            obj["message"] = boost::json::value_from(msg.message.value());
         }

         if (msg.field_messages.has_value())
         {
            obj["field_messages"] = boost::json::value_from(msg.field_messages.value());
         }
      }
   };
} // namespace spider2::api