#pragma once

#include <boost/unordered_map.hpp>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <boost/container/static_vector.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <coroutine>
#include <gsl/narrow>
#include <memory>
#include <stack>
#include <stop_token>
#include <thread>
#include <tuple>
#include <vector>

namespace spider2
{
   using string = std::string;
   using string_view = std::string_view;
   namespace chrono = std::chrono;
   namespace filesystem = boost::filesystem;
   using namespace std::literals;

   template <class T>
   using vector = std::vector<T>;

   template <class T>
   using optional = std::optional<T>;

   template <class K, class V>
   using unordered_map = boost::unordered_map<K, V>;

   struct string_equal
   {
      using is_transparent = std::true_type;

      inline bool operator()(std::string_view l, std::string_view r) const noexcept
      {
         return l == r;
      }
   };

   struct string_hash
   {
      using is_transparent = std::true_type;

      inline auto operator()(std::string_view str) const noexcept
      {
         return std::hash<std::string_view>()(str);
      }
   };

   template <class V>
   using string_key_unordered_map = boost::unordered_map<string, V, string_hash, string_equal>;

   using string_map = boost::unordered_map<string, string, string_hash, string_equal>;

   template <class K, class V>
   using map = std::map<K, V>;

   using stop_token = std::stop_token;
   using stop_source = std::stop_source;

   template <class F>
   using stop_callback = std::stop_callback<F>;

   template <class... T>
   using tuple = std::tuple<T...>;

   template <class F>
   stop_callback<F> make_stop_callback(stop_token token, F &&f)
   {
      return std::stop_callback{token, std::forward<F>(f)};
   }

   template <class... T>
   tuple<T...> make_tuple(T &&...t)
   {
      return std::tuple{std::forward<T>(t)...};
   }

   template <size_t Size>
   using uint8_fixed_stack = std::stack<boost::container::static_vector<std::uint8_t, Size>>;
} // namespace spider2