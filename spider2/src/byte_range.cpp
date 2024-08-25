//
// Created by jhrub on 28.02.2023.
//
#include "spider2/types/structs/byte_range.h"

#include "spider2/types/utils/ignore_result.h"

#include <boost/config/warning_disable.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <fmt/format.h>
#include <numeric>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(spider2::byte_range, offset_begin, offset_end)

auto spider2::parse_byte_ranges(string_view header_value) -> optional<byte_ranges>
{
   const auto integer_parser = x3::uint_parser<spider2::byte_range_int_type, 10, 1, -1>();
   const auto my_space = x3::char_(" \t");

   auto result = optional<byte_ranges>{byte_ranges{}};

   auto it = header_value.begin();
   const auto end = header_value.end();

   IGNORE_RESULT(x3::phrase_parse(it, end, "bytes=" >> ((integer_parser >> '-' >> -integer_parser) % ','), my_space,
                                  result.value()));

   if (it != end)
   {
      result.reset();
   }
   return result;
}
spider2::vector<spider2::string> spider2::prepare_byte_range_boundaries(const spider2::byte_ranges &ranges,
                                                                        std::uint64_t file_size,
                                                                        spider2::string_view boundary,
                                                                        spider2::string_view content_type)
{
   auto result = vector<string>{};
   result.reserve(ranges.size() + 1);

   auto first_boundary_prefix = "--"sv;
   auto middle_boundary_prefix = "\r\n--"sv;
   auto last_boundary_suffix = "--\r\n"sv;

   fmt::memory_buffer buffer;
   for (std::size_t i = 0, len = ranges.size(); i != len; ++i)
   {
      if (i == 0)
         buffer.append(first_boundary_prefix);
      else
         buffer.append(middle_boundary_prefix);

      buffer.append(boundary);
      fmt::format_to(std::back_inserter(buffer), "\r\nContent-Type:", content_type);

      byte_range const &range = ranges[i];
      fmt::format_to(std::back_inserter(buffer), "\r\nContent-Range: bytes {}-{}/{}\r\n\r\n", range.offset_begin,
                     range.offset_end.value_or(file_size - 1), file_size);

      result.push_back(fmt::to_string(buffer));
      buffer.clear();
   }
   fmt::format_to(std::back_inserter(buffer), "{}{}{}", middle_boundary_prefix, boundary, last_boundary_suffix);
   result.push_back(fmt::to_string(buffer));
   return result;
}
spider2::byte_range_int_type spider2::get_total_response_body_size(const spider2::byte_ranges &ranges,
                                                                   const spider2::vector<spider2::string> &boundaries,
                                                                   std::uint64_t file_size)
{
   const auto byte_ranges_size = std::accumulate(ranges.begin(), ranges.end(), std::uint64_t{0},
                                                 [=](std::uint64_t current, byte_range const &rng)
                                                 { return current + rng.get_length(file_size); });
   return std::accumulate(boundaries.begin(), boundaries.end(), byte_ranges_size,
                          [=](std::uint64_t current, string const &boundary) { return current + boundary.size(); });
}
