#include <catch.hpp>
#include <spider2/types/utils/function_traits.h>
#include <spider2/types/utils/tuple_algorithms.h>

using namespace spider2;

namespace test
{
   struct context_a
   {
      int a;
   };

   struct context_b
   {
      double a;
   };

   struct request
   {
      explicit request(int x)
         : x(x)
      {
      }

      int x;
   };

   struct missing
   {
   };
} // namespace test

TEST_CASE("tuple type contains", "[utils]")
{
   using namespace test;

   using tuple_type = std::tuple<context_a, context_b>;
   static_assert(tuple_contains_type_v<context_a, tuple_type> == true);
   static_assert(tuple_contains_type_v<context_b, tuple_type> == true);
   static_assert(tuple_contains_type_v<missing, tuple_type> == false);

   using const_tuple_type = std::tuple<const context_a, const context_b>;
   static_assert(tuple_contains_type_v<context_a, const_tuple_type> == false);
   static_assert(tuple_contains_type_v<const context_a, const_tuple_type> == true);

   using refs_tuple_type = std::tuple<context_a &&, const context_b &&>;
   static_assert(tuple_contains_type_v<context_a &&, refs_tuple_type> == true);
   static_assert(tuple_contains_type_v<const context_b &&, refs_tuple_type> == true);

   auto a = context_a{10};
   const auto b = context_b{10.0};
   [](auto &&... ctx)
   {
      auto tuple = std::forward_as_tuple(ctx...);
      static_assert(tuple_contains_type_v<context_a &, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const context_b &, decltype(tuple)>);

      auto &val = std::get<const_lval_ref_t<const context_b>>(tuple);
      auto &val1 = std::get<const_lval_ref_t<context_b>>(tuple);
      auto &val2 = std::get<const_lval_ref_t<const context_b &>>(tuple);
      auto &val3 = std::get<const_lval_ref_t<const context_b &&>>(tuple);

      static_assert(std::is_same_v<decltype(val), const context_b &>);
      static_assert(std::is_same_v<decltype(val1), const context_b &>);
      static_assert(std::is_same_v<decltype(val2), const context_b &>);
      static_assert(std::is_same_v<decltype(val3), const context_b &>);

      static_assert(tuple_contains_type_v<const_lval_ref_t<const context_b>, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const_lval_ref_t<context_b>, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const_lval_ref_t<context_b &>, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const_lval_ref_t<const context_b &>, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const_lval_ref_t<const context_b &&>, decltype(tuple)>);
      static_assert(tuple_contains_type_v<const_lval_ref_t<context_b &&>, decltype(tuple)>);
   }(a, b);
}

TEST_CASE("tuple type get", "[utils]")
{
   using namespace test;

   auto a = context_a{10};
   const auto b = context_b{10.0};

   [](auto &&... ctx)
   {
      auto tuple = std::forward_as_tuple(ctx...);

      static_assert(tuple_contains_type_v<const_lval_ref_t<const context_b>, decltype(tuple)> ||
                    tuple_contains_type_v<lval_ref_t<const context_b>, decltype(tuple)>,
                    "Tuple is not an argument tuple. It should be created by passing auto&&...args like "
                    "std::forward_as_tuple(std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...)");

      // required input is non-const
      {
         context_a &by_ref = fwd_args_get<context_a>(tuple);
         context_a const &by_const_ref = fwd_args_get<context_a>(tuple);
         context_a by_value = fwd_args_get<context_a>(tuple);

         REQUIRE(by_ref.a == 10);
         REQUIRE(by_const_ref.a == 10);
         REQUIRE(by_value.a == 10);
      }

      {
         context_a &by_ref = fwd_args_get<context_a &>(tuple);
         context_a const &by_const_ref = fwd_args_get<context_a &>(tuple);
         context_a by_value = fwd_args_get<context_a &>(tuple);

         REQUIRE(by_ref.a == 10);
         REQUIRE(by_const_ref.a == 10);
         REQUIRE(by_value.a == 10);
      }

      // required input is const
      {
         context_a const &by_const_ref = fwd_args_get<const context_a>(tuple);
         context_a by_value = fwd_args_get<const context_a>(tuple);

         REQUIRE(by_const_ref.a == 10);
         REQUIRE(by_value.a == 10);
      }

      {
         context_a const &by_const_ref = fwd_args_get<const context_a &>(tuple);
         context_a by_value = fwd_args_get<const context_a &>(tuple);

         REQUIRE(by_const_ref.a == 10);
         REQUIRE(by_value.a == 10);
      }

      {
         context_b const &by_const_ref = fwd_args_get<const context_b>(tuple);
         context_b by_value = fwd_args_get<const context_b>(tuple);

         REQUIRE(by_const_ref.a == 10.0);
         REQUIRE(by_value.a == 10.0);
      }

      {
         context_b const &by_const_ref = fwd_args_get<const context_b &>(tuple);
         context_b by_value = fwd_args_get<const context_b &>(tuple);

         REQUIRE(by_const_ref.a == 10.0);
         REQUIRE(by_value.a == 10.0);
      }

      {
         // uncomment to fail the compilation
         // context_b &by_value = fwd_args_get<context_b &>(tuple);
      }
   }(a, b);
}