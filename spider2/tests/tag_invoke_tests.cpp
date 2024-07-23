#include <catch.hpp>

#include <spider2/types/utils/tag_invoke.h>

namespace spider2
{
   inline constexpr struct extension_point_fn
   {
      template <class T>
      auto operator()(const T &v) const
      {
         return tag_invoke(*this, v);
      }
   } extension_point{};

   template <class T>
   int tag_invoke(tag_t<extension_point>, T const &)
   {
      return 0;
   }
} // namespace spider2

namespace other_lib
{
   struct custom_data_type
   {
    private:
      friend auto tag_invoke(spider2::tag_t<spider2::extension_point>, custom_data_type const &) -> int
      {
         return 43;
      }
   };

   struct my_type
   {
    private:
      friend auto tag_invoke(spider2::tag_t<spider2::extension_point>, my_type const &) -> int
      {
         return 42;
      }
   };

   struct not_customized
   {
   };
} // namespace other_lib

TEST_CASE("tag_invoke tests", "[tag_invoke]")
{
   {
      auto data = other_lib::my_type{};
      auto result = spider2::extension_point(data);

      REQUIRE(result == 42);
   }
   {
      auto data = other_lib::custom_data_type{};
      REQUIRE(spider2::extension_point(data) == 43);
   }

   {
      auto data = other_lib::not_customized{};
      REQUIRE(spider2::extension_point(data) == 0);
   }
}