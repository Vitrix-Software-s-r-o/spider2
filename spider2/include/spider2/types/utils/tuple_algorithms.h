//
// Created by jhrub on 19.03.2023.
//

#pragma once
#include <boost/hana.hpp>
#include <boost/hana/ext/std/tuple.hpp>

namespace spider2
{
   namespace hana = boost::hana;

   /**
    * This method takes two std::tuples and returns a new tuple that only contains
    * elements that are not in context_tuple and are located in args_tuple
    *
    * @param context_tuple
    * @param args_tuple
    * @return tuple of types that are not in context_tuple
    */
   auto get_missing_args_tuple(auto &&context_tuple, auto &&args_tuple)
   {
      return hana::remove_if(
          args_tuple, [&](auto t)
          { return hana::any_of(context_tuple, [&](auto &u) { return hana::decltype_(u) == hana::decltype_(t); }); });
   }

   template <typename... Ts>
   constexpr auto decay_tuple_types(std::tuple<Ts...> const &) -> std::tuple<std::decay_t<Ts>...>;

   template <class T>
   using decay_tuple = decltype(decay_tuple_types(std::declval<T>()));

   template <typename... Ts>
   constexpr auto remove_const_tuple_types(std::tuple<Ts...> const &) -> std::tuple<std::remove_const_t<Ts>...>;

   template <class T>
   using remove_const_tuple = decltype(decay_tuple_types(std::declval<T>()));

   template <typename Pred, typename Tuple>
   struct filter;

   template <typename t_Predicate, typename... Ts>
   struct filter<t_Predicate, std::tuple<Ts...>>
   {
      // If this element has to be kept, returns `std::tuple<Ts>`
      // Otherwise returns `std::tuple<>`
      template <class E>
      using t_filter_impl = std::conditional_t<t_Predicate::template condition<E>::value, std::tuple<E>, std::tuple<>>;

      // Determines the type that would be returned by `std::tuple_cat`
      //  if it were called with instances of the types reported by
      //  t_filter_impl for each element
      using type = decltype(std::tuple_cat(std::declval<t_filter_impl<Ts>>()...));
   };

   template <typename>
   struct is_std_tuple : std::false_type
   {
   };
   template <typename... T>
   struct is_std_tuple<std::tuple<T...>> : std::true_type
   {
   };

   template <typename T>
   constexpr bool is_std_tuple_v = is_std_tuple<T>::value;

   template <class U, class Tuple>
   struct tuple_contains_type
   {
      constexpr static bool value = false;
   };

   template <class U, class... T>
   struct tuple_contains_type<U, std::tuple<T...>>
   {
      constexpr static bool value = (std::is_same_v<U, T> || ...);
   };

   template <class U, class Tuple>
   constexpr bool tuple_contains_type_v = tuple_contains_type<U, Tuple>::value;

   template <class T>
   using const_lval_ref_t = const std::decay_t<T> &;
   template <class T>
   using lval_ref_t = std::decay_t<T> &;

   template <class T, class Tuple>
   auto fwd_args_get(Tuple &tuple) -> std::remove_reference_t<T> &
   {
      static_assert(tuple_contains_type_v<const_lval_ref_t<T>, Tuple> || tuple_contains_type_v<lval_ref_t<T>, Tuple>,
                    "Tuple is not an argument tuple. It should be created by passing auto&&...args like "
                    "std::forward_as_tuple(ctx...)");

      if constexpr (tuple_contains_type_v<const_lval_ref_t<T>, Tuple>)
      {
         static_assert(
             !std::is_reference_v<T> || (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>),
             "Add const to the argument type. The argument type is not a const reference type or value type. ");

         return std::get<const_lval_ref_t<T>>(tuple);
      }
      else if constexpr (tuple_contains_type_v<lval_ref_t<T>, Tuple>)
      {
         return std::get<lval_ref_t<T>>(tuple);
      }

      throw std::runtime_error(
          "Type not found in tuple. This must be a bug in the compiler if the static assert holds this is impossible.");
   }
} // namespace spider2