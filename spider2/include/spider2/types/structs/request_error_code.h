//
// Created by jhrub on 17.12.2022.
//

#pragma once

#include <boost/system.hpp>

namespace spider2
{
   enum class request_error_code
   {
      ok = 0, // 0 should not represent an error
      header_read_error = 1,
      body_read_error = 2,
      socket_stolen = 3,

      not_implemented = 4,
      api_input_data_error = 5,
      body_read_error_no_parser = 6
   };

   namespace detail
   {
      // Define a custom error code category derived from boost::system::error_category
      class spider_error_category : public boost::system::error_category
      {
       public:
         // Return a short descriptive name for the category
         inline virtual const char *name() const noexcept override final
         {
            return "spider error";
         }

         inline
             // Return what each enum means in text
             virtual std::string
             message(int c) const override final
         {
            switch (static_cast<request_error_code>(c))
            {
            case request_error_code::ok:
               return "ok";
            case request_error_code::header_read_error:
               return "header read error";
            case request_error_code::body_read_error:
               return "body read error";
            case request_error_code::socket_stolen:
               return "socket stolen";
            case request_error_code::not_implemented:
               return "not implemented";
            case request_error_code::api_input_data_error:
               return "api input data error";
            case request_error_code::body_read_error_no_parser:
               return "body read error no parser";
            default:
               return "unknown";
            }
         }

         inline virtual boost::system::error_condition default_error_condition(int c) const noexcept override final
         {
            switch (static_cast<request_error_code>(c))
            {
            case request_error_code::header_read_error:
               return make_error_condition(boost::system::errc::state_not_recoverable);

            default:
               // I have no mapping for this code
               return boost::system::error_condition(c, *this);
            }
         }
      };
   } // namespace detail
} // namespace spider2
namespace boost::system
{
   // Tell the C++ 11 STL metaprogramming that enum ConversionErrc
   // is registered with the standard error code system
   template <>
   struct is_error_code_enum<spider2::request_error_code> : std::true_type
   {
   };
} // namespace boost::system

namespace spider2
{
   inline const spider2::detail::spider_error_category &spider_error_category()
   {
      static spider2::detail::spider_error_category c;
      return c;
   }

   inline boost::system::error_code make_error_code(request_error_code e)
   {
      return {static_cast<int>(e), spider_error_category()};
   }
} // namespace spider2