#pragma once

#include "../platform.h"
#include <boost/date_time.hpp>
#include <chrono>
#include <string>
namespace spider2
{
   bool try_parse_http_date(boost::posix_time::ptime &time, string_view date_time_string) noexcept;

   bool try_parse_http_date(std::chrono::system_clock::time_point &result, string_view date_time_string) noexcept;

   string format_http_date(const std::chrono::system_clock::time_point &time);
   string format_http_date(const boost::posix_time::ptime &time);
   string format_etag_from_time_point(const std::chrono::system_clock::time_point &time);
   string format_etag_from_time_point(const boost::posix_time::ptime &time);

   chrono::system_clock::time_point to_std(const boost::posix_time::ptime &time);
   boost::posix_time::ptime to_boost(const std::chrono::system_clock::time_point &time);

} // namespace spider2