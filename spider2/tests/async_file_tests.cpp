//
// Created by jhrub on 24.02.2023.
//

#include <catch.hpp>

#include <fstream>
#include <spider2/spider2.h>
#include <spider2/types.h>

using namespace spider2;

io::awaitable<void> tcp_serve(std::stop_token token, tcp::endpoint endpoint, auto connection_handler)
{
   auto executor = co_await this_coro::executor;

   while (!token.stop_requested())
   {
      auto [ec, socket] = co_await accept_tcp(token, endpoint);
      if (!ec)
      {
         co_spawn(executor, connection_handler(token, std::move(socket)), io::detached);
      }
   }
}

std::string make_string(boost::asio::streambuf &streambuf)
{
   return std::string{std::istreambuf_iterator<char>(&streambuf), std::istreambuf_iterator<char>()};
}

TEST_CASE("async file transfer", "[async]")
{

   auto test_string = string{"test content"};
#if defined(BOOST_WINDOWS)
   filesystem::path temp_file = std::tmpnam(nullptr);
#else
   filesystem::path temp_file = filesystem::path{"/tmp"} / spider2::guid::generate_unsafe_uuid_string();
#endif
   {
      std::ofstream os{temp_file.string()};
      os << test_string;
   }

   auto executor = boost::asio::io_context{};

   bool begin_sent_file = false;
   auto stop_source = std::stop_source{};
   auto tcp_endpoint = tcp::endpoint{io::ip::address_v4::loopback(), 3000};
   io::co_spawn(executor,
                tcp_serve(stop_source.get_token(), tcp_endpoint,
                          [&](std::stop_token token, tcp::socket socket) -> io::awaitable<void>
                          {
                             auto executor = co_await io::this_coro::executor;
                             auto handle = async_file_io::open_binary_file(executor, temp_file);
                             if (handle.has_value())
                             {
                                begin_sent_file = true;
                                auto [ec, bytes] = co_await async_file_io::send_file(*handle, socket);
                                if (ec)
                                {
                                   std::cout << ec.message() << std::endl;
                                }
                             }
                          }),
                io::detached);

   //   std::this_thread::sleep_for(std::chrono::seconds{1});
   auto result = io::co_spawn(
       executor,
       [&]() -> io::awaitable<std::string>
       {
          auto executor = co_await io::this_coro::executor;
          auto client_connection = tcp::socket{executor, tcp_endpoint.protocol()};

          while (!stop_source.stop_requested())
          {
             auto [ec] = co_await client_connection.async_connect(tcp_endpoint, ioe::as_tuple(io::use_awaitable));
             if (!ec)
             {
                auto buffer = io::streambuf{};
                auto [read_ec, bytes] = co_await io::async_read(client_connection, buffer.prepare(test_string.size()),
                                                                ioe::as_tuple(io::use_awaitable));

                buffer.commit(bytes);
                co_return make_string(buffer);
             }
             co_return std::string{};
          }
       },
       io::use_future);

   auto thread = std::jthread{[&]
                              {
                                 while (!executor.stopped())
                                    executor.run();
                              }};

   auto result_string = result.get();
   stop_source.request_stop();
   executor.stop();
   REQUIRE(result_string == test_string);
   REQUIRE(begin_sent_file == true);
}