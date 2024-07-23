//
// Created by jhrub on 3/10/2024.
//

#pragma once
#include <ranges>
#include <spider2/types.h>

#include "from_query.h"
#include <boost/json.hpp>

namespace spider2::api
{

   inline constexpr struct try_query_args_parse_value_fn
   {
      template <class T>
      auto operator()(string_view key, string_view value, T &output) const noexcept -> bool
      {
         return tag_invoke(*this, key, value, output);
      }

      template <class T>
      static auto try_parse_value_using_lexical_cast(string_view value, T &out) noexcept -> bool
      {
         try
         {
            out = boost::lexical_cast<T>(value);
            return true;
         }
         catch (...)
         {
            return false;
         }
      }

      template <class T>
      friend auto tag_invoke(try_query_args_parse_value_fn, string_view, string_view value, T &out) noexcept -> bool
      {
         return try_parse_value_using_lexical_cast(value, out);
      }

   } try_query_args_parse_value{};

   /**
    * @brief query_args is a helper struct that is used to parse the query string of a request.
    * Use it if youd don't want to parse the query string yourself.
    *
    * It uses boost::json::object to store the query parameters.
    * It could be use to parse request body with content type "application/x-www-form-urlencoded" and
    * "application/json".
    */
   struct query_args
   {
      boost::json::object query_map;

      /**
       * @brief Try to interpret value as given Type T and return its value on success.
       * If the key is not present or the value cannot be interpreted as T, an empty optional is returned.
       *
       * @tparam T - the type of the value
       * @param key - the key of the query parameter
       * @return interpreted value or empty optional
       */
      template <class T>
      auto as_value(const string &key) const -> optional<T>
      {
         if (const auto it = query_map.find(key); it != query_map.end())
         {
            return this->try_parse_value<T>(key, it->value());
         }
         return {};
      }

      /**
       * @brief  Try to interpret value as vector of  T.
       * It uses default predicate (boost::is_any_of(",")).
       * It can parse values:
       *    - "1,2,3" -> [1, 2, 3]
       * @tparam T - output type
       * @param key - query key
       * @return vector of T - if no value is present or interpreted, an empty vector is returned
       */
      template <class T>
      auto as_vector(const string &key) const -> vector<T>
      {
         return as_vector<T>(key, boost::is_any_of(","));
      }

      template <class T, class Pred>
      auto as_vector(const string &key, Pred pred) const -> vector<T>
      {
         if (const auto it = query_map.find(key); it != query_map.end())
         {
            auto kind = it->value().kind();
            if (kind == boost::json::kind::string)
            {
               auto str_vec = vector<string>{};
               boost::split(str_vec, it->value().as_string(), pred);

               if constexpr (std::is_same_v<string, T>)
               {
                  return str_vec;
               }

               auto out_vec = vector<T>{};
               out_vec.reserve(str_vec.size());

               auto transform_range =
                   str_vec | std::views::transform([&](const string &str) { return try_parse_value<T>(key, str); }) |
                   std::views::filter([](const auto &opt) { return opt.has_value(); }) |
                   std::views::transform([](const auto &opt) { return opt.value(); });
               ;

               out_vec.insert(out_vec.end(), transform_range.begin(), transform_range.end());
               return out_vec;
            }
            else if (kind == boost::json::kind::array)
            {
               auto &arr = it->value().as_array();
               auto out_vec = vector<T>{};
               out_vec.reserve(arr.size());

               auto transform_range =
                   arr |
                   std::views::transform([&](const boost::json::value &val) { return try_parse_value<T>(key, val); }) |
                   std::views::filter([](const auto &opt) { return opt.has_value(); }) |
                   std::views::transform([](const auto &opt) { return opt.value(); });

               out_vec.insert(out_vec.end(), transform_range.begin(), transform_range.end());
               return out_vec;
            }
         }

         return {};
      }

      friend auto tag_invoke(tag_t<parse_query>, string_view query, query_args &output) -> error_code
      {
         if (url::parse_query(query.begin(), query.end(),
                              [&](string &key, string &value)
                              {
                                 auto [it, inserted] = output.query_map.emplace(key, value);
                                 if (!inserted)
                                 {
                                    if (it->value().kind() == boost::json::kind::array)
                                    {
                                       auto &arr = it->value().as_array();
                                       arr.emplace_back(value);
                                    }
                                    else
                                    {
                                       auto arr = boost::json::array{};
                                       arr.push_back(it->value());
                                       arr.emplace_back(value);
                                       it->value() = arr;
                                    }
                                 }
                              }))
         {
            return {};
         }
         return make_error_code(request_error_code::api_input_data_error);
      }

      inline friend auto tag_invoke(boost::json::value_to_tag<query_args>, boost::json::value const &jv) -> query_args
      {
         if (jv.is_object())
         {
            return {.query_map = jv.as_object()};
         }
         return {};
      }

    private:
      template <class T>
      optional<T> try_parse_value(const string &key, const string &value) const
      {
         if constexpr (std::is_same_v<string, T>)
         {
            return {value};
         }
         else if constexpr (std::is_same_v<bool, T>)
         {
            if (value == "1" || value == "true" || value == "True" || value == "TRUE" || value == "on" ||
                value == "ON" || value == "On" || value == "yes" || value == "YES" || value == "Yes")
               return true;

            if (value == "0" || value == "false" || value == "False" || value == "FALSE" || value == "off" ||
                value == "OFF" || value == "Off" || value == "no" || value == "NO" || value == "No")
               return false;

            return {};
         }
         else
         {
            if (auto out_value = T{}; try_query_args_parse_value(key, value, out_value))
            {
               return {std::move(out_value)};
            }
            return {};
         }
      }

      template <class T>
      optional<T> try_parse_value(const string &key, const boost::json::value &value) const
      {
         const auto kind = value.kind();

         if constexpr (std::is_same_v<string, T>)
         {
            if (kind == boost::json::kind::string)
            {
               auto &str = value.as_string();
               return {{str.begin(), str.end()}};
            }
            else
            {
               return boost::json::serialize(value);
            }
         }
         if constexpr (std::is_same_v<T, bool>)
         {
            if (value.is_bool())
            {
               return value.as_bool();
            }
            if (value.is_number())
            {
               return value.to_number<double>() != 0.0;
            }

            if (value.is_string())
            {
               auto &str = value.as_string();
               return this->try_parse_value<T>(key, std::string{str.begin(), str.end()});
            }

            return {};
         }
         if constexpr (!std::is_same_v<T, bool> && (std::is_integral_v<T> || std::is_floating_point_v<T>))
         {
            if (value.is_number())
            {
               return value.to_number<T>();
            }
            else if (value.is_string())
            {
               auto &str = value.as_string();
               auto std_str = string{str.begin(), str.end()};
               return this->try_parse_value<T>(key, std_str);
            }
         }

         if (value.is_string())
         {
            auto &str = value.as_string();
            auto std_str = string{str.begin(), str.end()};
            return this->try_parse_value<T>(key, std_str);
         }

         return {};
      }
   };
} // namespace spider2::api