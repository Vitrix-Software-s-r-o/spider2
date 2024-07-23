//
// Created by jhrub on 04.01.2023.
//

#pragma once

#include "function_traits.h"
#include "spider2/types/platform/asio.h"
#include "tuple_algorithms.h"

#include <type_traits>

namespace spider2
{

   struct any_function_adapter_return_value_policy
   {
      struct honor_return_type
      {
      };
      struct convert_to_async
      {
      };
   };

   template <class Policy, class Fun, class OutputTuple, class InputTuple>
   struct any_function_adapter;

   template <class Fun, class... OutputArgs, class... InputArgs>
   struct any_function_adapter<any_function_adapter_return_value_policy::honor_return_type, Fun,
                               std::tuple<OutputArgs...>, std::tuple<InputArgs...>>
   {

      static auto apply(Fun fun, std::tuple<InputArgs...> &inputArgs)
      {
         if constexpr (std::is_void_v<typename function_traits<Fun>::return_type>)
         {
            fun(fwd_args_get<OutputArgs>(inputArgs)...);
         }
         else
         {
            return fun(fwd_args_get<OutputArgs>(inputArgs)...);
         }
      }
   };

   template <class Fun, class... OutputArgs, class... InputArgs>
   struct any_function_adapter<any_function_adapter_return_value_policy::convert_to_async, Fun,
                               std::tuple<OutputArgs...>, std::tuple<InputArgs...>>
   {

      static auto apply(Fun fun, std::tuple<InputArgs...> &inputArgs)
      {
         using awaitable_tag = is_awaitable_result_t<typename function_traits<Fun>::return_type>;

         return any_function_adapter::apply_internal(awaitable_tag{}, fun, inputArgs);
      }

    private:
      /// This is an awaitable return type
      static auto apply_internal(std::true_type, Fun &fun, std::tuple<InputArgs...> &inputArgs)
      {
         return fun(fwd_args_get<OutputArgs>(inputArgs)...);
      }

      /// This is not an awaitable return type converted to awaitable
      static auto apply_internal(std::false_type, Fun &fun, std::tuple<InputArgs...> &inputArgs)
      {
         auto handler_args_tuple = std::tuple_cat(std::forward_as_tuple(fun), inputArgs);

         if constexpr (std::is_void_v<typename function_traits<Fun>::return_type>)
         {
            auto handler = [](Fun &fun, auto &&...args) -> io::awaitable<void>
            {
               auto input_args = std::forward_as_tuple(args...);
               fun(fwd_args_get<OutputArgs>(input_args)...);
               co_return;
            };

            return std::apply(handler, handler_args_tuple);
         }
         else
         {
            auto handler = [](Fun &fun, auto &&...args) -> io::awaitable<typename function_traits<Fun>::return_type>
            {
               auto input_args = std::forward_as_tuple(args...);
               co_return fun(fwd_args_get<OutputArgs>(input_args)...);
            };
            return std::apply(handler, handler_args_tuple);
         }
      }
   };

   /**
    * This function wraps any callable function to pick arguments from the set as you pass it.
    * Argument types must be unique amogs the parameters.
    *
    * In case of any_function_adapter_return_value_policy::honor_return_type, the return type of the passed function is
    * honored. In case of any_function_adapter_return_value_policy::convert_to_async, the return type of the passed
    * function is converted to io::awaitable<return_type>
    *
    * auto fun = wrap_any_function([](int x, double y){
    *   std::cout << x << std::endln;
    *   std::cout << y << std::endln;
    * });
    *
    * struct not_used
    * {};
    *
    * Usage:
    * fun(not_used{}, 3.333, 10);
    *
    * It prints:
    * 10
    * 3.333
    *
    */

   template <class Fun>
   auto wrap_any_function(Fun &&fun)
   {
      return [fun = std::forward<std::decay_t<Fun>>(fun)](auto &&...ctx)
      {
         using arg_tuple = typename function_traits<Fun>::arguments;
         using return_type = typename function_traits<Fun>::return_type;

         auto current_args = std::forward_as_tuple(ctx...);

         using adapter_type = any_function_adapter<any_function_adapter_return_value_policy::honor_return_type, Fun,
                                                   arg_tuple, decltype(current_args)>;

         if constexpr (std::is_void_v<return_type>)
         {
            adapter_type::apply(fun, current_args);
         }
         else
         {
            return adapter_type::apply(fun, current_args);
         }
      };
   }

   template <class Fun>
   auto wrap_any_function_async(Fun &&fun)
   {
      return [fun = std::forward<std::decay_t<Fun>>(fun)](auto &&...ctx)
      {
         using arg_tuple = typename function_traits<Fun>::arguments;
         auto current_args = std::forward_as_tuple(ctx...);

         using adapter_type = any_function_adapter<any_function_adapter_return_value_policy::convert_to_async, Fun,
                                                   arg_tuple, decltype(current_args)>;

         return adapter_type::apply(fun, current_args);
      };
   };
} // namespace spider2