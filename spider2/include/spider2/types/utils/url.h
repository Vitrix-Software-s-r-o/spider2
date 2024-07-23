#pragma once
#include <string>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace spider2::url
{

   template <class IteratorT>
   std::string encode(IteratorT begin, IteratorT end)
   {
      using namespace std;
      std::stringstream buffer{};
      buffer.fill('0');
      buffer << hex;

      for (; begin != end; ++begin)
      {
         const auto c = static_cast<unsigned char>(*begin);

         // Keep alphanumeric and other accepted characters intact
         if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
         {
            buffer << c;
         }
         else
         {
            // Any other characters are percent-encoded
            buffer << uppercase;
            buffer << '%' << setw(2) << static_cast<int>(static_cast<unsigned char>(c));
            buffer << nouppercase;
         }
      }

      return buffer.str();
   }

   template <class ContainerT>
   std::string encode(const ContainerT &container)
   {
      using namespace std;
      return encode(begin(container), end(container));
   }

   template <class CharT>
   int find_hex_idx(CharT c) noexcept
   {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      return -1;
   }

   template <class IteratorT>
   int parse_octet(IteratorT &begin, IteratorT end) noexcept
   {
      const size_t len = end - begin;
      if (len == 0) return -1;

      const int first = find_hex_idx(*begin++);
      if (first < 0 || begin == end) return first;

      const int second = find_hex_idx(*begin++);
      if (second < 0) return second;

      return second + (16 * first);
   }

   template <class OutputFunctT, class OctetOuputFunctT, class IteratorT>
   bool try_decode(OutputFunctT outputFunct, OctetOuputFunctT octetOutput, IteratorT pos, IteratorT end);

   template <class StringT, class IteratorT>
   bool try_decode(StringT &output, IteratorT begin, IteratorT end)
   {
      return try_decode([&](IteratorT b, IteratorT e) { std::copy(b, e, std::back_inserter(output)); },
                        [&](std::uint8_t ch) { output += static_cast<typename StringT::value_type>(ch); }, begin, end);
   }

   template <class OutputFunctT, class OctetOuputFunctT, class IteratorT>
   bool try_decode(OutputFunctT outputFunct, OctetOuputFunctT octetOutput, IteratorT pos, IteratorT end)
   {
      while (pos != end)
      {
         IteratorT it = std::find_if(pos, end, boost::is_any_of("%+"));

         if (it != pos)
         {
            outputFunct(pos, it);
            pos = it;
         }

         if (pos != end)
         {
            if (*pos++ == '+')
            {
               octetOutput(' ');
            }
            else
            {
               auto octet = parse_octet(pos, end);
               if (octet < 0) return false;

               octetOutput(static_cast<std::uint8_t>(octet));
            }
         }
      }

      return true;
   }

   template <typename IteratorT, typename FuncType, typename StringT = std::string>
   bool parse_query(IteratorT begin, IteratorT end, FuncType callback)
   {
      char seperators[] = {'&', '?', ';', '='};
      auto pos = begin;

      StringT name;
      StringT value;

      if (pos != end && (*pos == '&' || *pos == '?' || *pos == ';' || *pos == ' '))
      {
         ++pos;
      }

      while (pos != end)
      {
         if (*pos == ' ')
         {
            ++pos;
            continue;
         }

         auto separator_pos = std::find_first_of(pos, end, seperators, seperators + sizeof(seperators));
         bool is_name = name.empty();

         if (is_name && !try_decode(name, pos, separator_pos))
         {
            return false;
         }
         else if (!is_name && !try_decode(value, pos, separator_pos))
         {
            return false;
         }

         if (!is_name || separator_pos == end || *separator_pos == '&' || *separator_pos == '?' ||
             *separator_pos == ';')
         {
            callback(name, value);
            name.clear();
            value.clear();
         }

         if (separator_pos == end) break;

         pos = separator_pos + 1;
      }

      return true;
   }

} // namespace spider2::url
