#include "spider2/types/utils/etag.h"
#include <base64/base64.h>
#include <fmt/format.h>

spider2::string spider2::etag::make_etag(chrono::system_clock::time_point last_write_time, std::uint64_t value) noexcept
{
   auto result = static_cast<std::uint64_t>(
                     chrono::duration_cast<chrono::milliseconds>(last_write_time.time_since_epoch()).count()) ^
                 value;

   return fmt::format("\"{}\"", base64_encode(reinterpret_cast<const unsigned char *>(&result), sizeof(result)));
}
