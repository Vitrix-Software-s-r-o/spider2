#include "spider2/types/utils/http_date.h"
#include "base64.h"
#include <sstream>

using namespace boost;
using namespace boost::date_time;
using namespace boost::local_time;

bool spider2::try_parse_http_date(boost::posix_time::ptime &result, spider2::string_view value) noexcept
{
   try
   {
      if (!value.empty())
      {
         auto input_facet = std::make_unique<posix_time::time_input_facet>();
         input_facet->format("%a, %d %b %Y %H:%M:%S GMT");

         std::stringstream ss;
         ss.imbue(std::locale(ss.getloc(), input_facet.release()));
         ss << value;

         posix_time::ptime out;
         ss >> out;
         if (ss.bad()) return false;

         result = out;
         return true;
      }
   }
   catch (...)
   {
      return false;
   }
   return false;
}

bool spider2::try_parse_http_date(std::chrono::system_clock::time_point &result, spider2::string_view value) noexcept
{
   posix_time::ptime out;
   if (try_parse_http_date(out, value))
   {
      result = to_std(out);
      return true;
   }

   return false;
}

std::string spider2::format_http_date(const std::chrono::system_clock::time_point &time)
{
   return format_http_date(to_boost(time));
}

std::string spider2::format_http_date(const boost::posix_time::ptime &time)
{
   std::stringstream ss;
   auto facet = std::make_unique<posix_time::time_facet>();
   facet->format("%a, %d %b %Y %H:%M:%S GMT");

   ss.imbue(std::locale(std::locale::classic(), facet.release()));
   ss << time;

   return ss.str();
}

std::string spider2::format_etag_from_time_point(const std::chrono::system_clock::time_point &time)
{
   auto time_time_t = std::chrono::system_clock::to_time_t(time);
   return base64::encode({reinterpret_cast<const char *>(&time_time_t), sizeof(time_time_t)});
}

std::string spider2::format_etag_from_time_point(const boost::posix_time::ptime &time)
{
   auto time_time_t = to_time_t(time);
   return base64::encode({reinterpret_cast<const char *>(&time_time_t), sizeof(time_time_t)});
}

std::chrono::system_clock::time_point spider2::to_std(const boost::posix_time::ptime &time)
{
   return std::chrono::system_clock::from_time_t(to_time_t(time));
}

boost::posix_time::ptime spider2::to_boost(const std::chrono::system_clock::time_point &time)
{
   return boost::posix_time::from_time_t(std::chrono::system_clock::to_time_t(time));
}
