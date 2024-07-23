//
// Created by jhrub on 11.12.2022.
//

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION <= 107900
#include <boost/asio/experimental/as_tuple.hpp>
#else
#include <boost/asio/as_tuple.hpp>
#endif
#include <boost/asio/strand.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace spider2
{
   namespace io = boost::asio;
#if BOOST_VERSION <= 107900
   namespace ioe = io::experimental;
#else
   namespace ioe = io;
#endif

   namespace this_coro = io::this_coro;
   using tcp = io::ip::tcp;
   using ip_address = io::ip::address;
   using thread_pool = io::thread_pool;

   using boost::system::error_code;

   template <class R>
   struct is_awaitable_result : std::false_type
   {
      ;
   };

   template <class T>
   struct is_awaitable_result<io::awaitable<T>> : std::true_type
   {
   };

   template <class R>
   using is_awaitable_result_t = typename is_awaitable_result<R>::type;

   template <class R>
   constexpr bool is_awaitable_result_v = is_awaitable_result<R>::value;

   template <class Awaitable>
   struct get_result_type
   {
      using type = void;
   };

   template <class T>
   struct get_result_type<io::awaitable<T>>
   {
      using type = T;
   };

#ifdef BOOST_WINDOWS_API
   using file_handle = boost::asio::windows::random_access_handle;
   using reuse_port_and_address = boost::asio::socket_base::reuse_address;
#else
   using file_handle = boost::asio::posix::stream_descriptor;
   using reuse_port_and_address =
       boost::asio::detail::socket_option::boolean<BOOST_ASIO_OS_DEF(SOL_SOCKET),
                                                   BOOST_ASIO_OS_DEF(SO_REUSEADDR | SO_REUSEPORT)>;
#endif

   constexpr auto use_tuple_awaitable = ioe::as_tuple(io::use_awaitable);

} // namespace spider2