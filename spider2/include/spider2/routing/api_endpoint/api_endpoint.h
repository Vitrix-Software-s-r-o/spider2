//
// Created by jhrub on 29.11.2022.
//

#pragma once
#include "format_api_response.h"
#include "parse_api_request.h"
#include "read_api_request.h"

#include "../../types.h"

namespace spider2
{
   template <class ApiFun, class WrappedFun>
   struct api_endpoint
   {
      struct filter_out_request_arg
      {
         template <class T>
         struct condition
         {
            constexpr static bool value = !std::is_same_v<request, T> && std::is_default_constructible_v<T>;
         };
      };

      using arguments_tuple =
          typename filter<filter_out_request_arg, decay_tuple<typename function_traits<ApiFun>::arguments>>::type;
      using return_type = typename function_traits<ApiFun>::return_type;

      WrappedFun fun;

      explicit api_endpoint(WrappedFun &&_fun) : fun(std::forward<WrappedFun>(_fun)) {}

      template <class... T>
      auto operator()(T &&...args) const -> await_response
      {
         auto context_args = std::forward_as_tuple(std::forward<T>(args)...);

         // get_missing_args_tuple returns a tuple of types that are not in context_args and are located in
         // arguments_tuple
         auto data_args = get_missing_args_tuple(context_args, arguments_tuple{});

         // get spider2::request from context_args
         auto &req = std::get<request &>(context_args);

         // this call will read the request from the socket to the request body message such as http::string_body
         // can be overriden by custom types handling by implementation custom auto tag_invoke(tag_t<read_api_request>,
         // req, input...) -> io::awaitable<error_code>
         auto ec = co_await std::apply([&](auto &...input) -> io::awaitable<error_code>
                                       { return read_api_request(req, input...); }, data_args);
         if (ec)
         {
            co_return response::return_string(status::bad_request, ec.message());
         }

         bool result = std::apply([&](auto &...input) mutable -> bool
                                  { return (... && this->parse_api_input_from_request(req, ec, input)); }, data_args);
         if (!result && ec)
         {
            co_return response::return_string(status::bad_request, ec.message());
         }

         if constexpr (is_awaitable_result_v<return_type>)
         {
            // awaitable branch - call the function and co_await the result
            if constexpr (std::is_same_v<void, typename get_result_type<return_type>::type>)
            {
               co_await std::apply(fun, std::tuple_cat(context_args, data_args));
               co_return response::return_string(status::ok, "ok");
            }
            else
            {
               co_return format_api_response(req, co_await std::apply(fun, std::tuple_cat(context_args, data_args)));
            }
         }
         else
         {
            // potentialy blocking branch - call the function and return the result
            if constexpr (std::is_same_v<void, return_type>)
            {
               std::apply(fun, std::tuple_cat(context_args, data_args));
               co_return response::return_string(status::ok, "ok");
            }
            else
            {
               co_return format_api_response(req, std::apply(fun, std::tuple_cat(context_args, data_args)));
            }
         }
      }

    private:
      auto parse_api_input_from_request(request &req, error_code &out_error, auto &current) const -> bool
      {
         auto ec = parse_api_request(req, current);
         if (ec)
         {
            out_error = ec;
            return false;
         }

         return true;
      }
   };

   template <class Fun>
   auto wrap_api_function(Fun &&fun)
   {
      auto &&wrapped = wrap_any_function(std::forward<Fun>(fun));
      using wrapped_t = std::decay_t<decltype(wrapped)>;
      return api_endpoint<Fun, wrapped_t>{std::forward<wrapped_t>(wrapped)};
   }

} // namespace spider2