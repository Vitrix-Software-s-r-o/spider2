#pragma once
#include <cstdint>
#include <spider2/types.h>
#include <variant>

#include <boost/json.hpp>
#include <functional>
namespace spider2::api
{
   struct flat_value : std::variant<string, std::int64_t, double, bool>
   {
      using std::variant<string, std::int64_t, double, bool>::variant;

      friend void tag_invoke(boost::json::value_from_tag, boost::json::value &jv, flat_value const &v)
      {
         std::visit([&jv](auto const &value) { jv = value; }, v);
      }
   };

} // namespace spider2::api
