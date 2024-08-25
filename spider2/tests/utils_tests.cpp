#include <catch.hpp>
#include <spider2/types.h>

using namespace spider2;

struct context_a
{
   int a;
};

struct context_b
{
   double a;
};

const auto first = [](auto fun)
{
   return [=](auto &&... ctx)
   {
      const auto new_context = context_a{10};
      fun(std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)..., new_context);
   };
};

const auto second = [](auto fun)
{
   return [=](auto &&... ctx)
   {
      const auto &a_context = get_arg<const context_a>(std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);

      const auto new_context = context_b{static_cast<double>(a_context.a)};
      fun(std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)..., new_context);
   };
};

const auto second_without_context_from_first = [](auto fun)
{
   return [=](auto &&... ctx)
   {
      const auto new_context = context_b{11.5};
      fun(std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)..., new_context);
   };
};

TEST_CASE("test wrap_any_function", "[utils]")
{

   auto pipeline = first(second(wrap_any_function([](int x, context_b const &b, context_a const &a)
   {
      REQUIRE(x == 10);
      REQUIRE(a.a == 10);
      REQUIRE(b.a == 10.0);

   })));

   int x = 10;
   pipeline(x);

   auto fun = wrap_any_function([](int x, double y)
   {
      REQUIRE(x == 10);
      REQUIRE(y == 3.333);
   });
   struct not_used
   {
   };

   fun(not_used{}, 3.333, 10);
}

TEST_CASE("test middleware chain", "[utils]")
{
   auto chain = middleware_chain_builder<>{} | first | second;
   auto fun = chain([](context_a const &a, context_b const &b)
   {
      REQUIRE(a.a == 10);
      REQUIRE(b.a == 10.0);
   });

   fun();

   auto chain2 = middleware_chain_builder<>{} | second_without_context_from_first | first;
   auto fun2 = chain2([](context_b const &b, context_a const &a)
   {
      REQUIRE(a.a == 10);
      REQUIRE(b.a == 11.5);
   });

   fun2();
}

TEST_CASE("static_lookup_table")
{
   using namespace std::string_view_literals;

   SECTION("linear search")
   {
      auto table = static_lookup_table{
         std::array{std::pair{"key"sv, "value"sv}, std::pair{"other key"sv, "other value"sv}}};
      REQUIRE(table.lookup("key"sv) == "value"sv);
      REQUIRE(table.lookup("other key"sv) == "other value"sv);
   }

   SECTION("binary search")
   {
      auto init = std::array<std::pair<int, std::size_t>, 200>{};
      for (std::size_t i = 0; i != init.size(); ++i)
      {
         init[i] = std::pair{static_cast<int>(init.size() - i), i};
      }
      auto table = static_lookup_table{init};

      REQUIRE(table.lookup(1) == 199);
      REQUIRE(table.lookup(100) == 100);
      REQUIRE(table.lookup(300).has_value() == false);

   }
}
