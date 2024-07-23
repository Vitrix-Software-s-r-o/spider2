#pragma once

#include "../platform.h"

namespace spider2
{

   using byte_range_int_type = std::uint64_t;

   struct byte_range
   {
      byte_range_int_type offset_begin;

      // boost version is here because of the boost spirit
      boost::optional<byte_range_int_type> offset_end;

      inline byte_range_int_type get_length(byte_range_int_type file_size) const
      {
         return offset_end.value_or(file_size - 1) + 1 - offset_begin;
      }
   };

   using byte_ranges = vector<byte_range>;

   auto parse_byte_ranges(string_view header_value) -> optional<byte_ranges>;

   inline bool validate_byte_ranges(std::uint64_t size, const byte_ranges &ranges) noexcept
   {
      for (auto &range : ranges)
      {
         if (range.offset_begin >= size)
         {
            return false;
         }

         if (range.offset_end)
         {
            if (range.offset_end.value() >= size) return false;
            if (range.offset_end.value() < range.offset_begin) return false;
         }
      }

      return true;
   }

   vector<string> prepare_byte_range_boundaries(byte_ranges const &ranges, std::uint64_t file_size,
                                                string_view boundary, string_view content_type);

   byte_range_int_type get_total_response_body_size(byte_ranges const &ranges, vector<string> const &boundaries,
                                                    std::uint64_t file_size);

} // namespace spider2