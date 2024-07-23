//
// Created by jhrub on 3/9/2024.
//

#pragma once
namespace spider2::api
{
   template <class T>
   struct is_api_result
   {
      constexpr static std::false_type value{};
   };

   template <class T>
   constexpr bool is_api_result_v = is_api_result<T>::value;

   template <class T>
   concept api_result_c = is_api_result_v<T>;
} // namespace spider2::api