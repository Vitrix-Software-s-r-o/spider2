#include <catch.hpp>
#include <spider2/types/utils/multipart_message_parser.h>

using namespace spider2;
struct test_message_handler
{
   auto on_part_begin(http::fields fields, error_code &ec) -> void
   {
      parts.emplace_back(fields, std::string{});
   }
   auto on_part_data(std::span<const std::byte> data, error_code &ec) -> void
   {
      REQUIRE(parts.empty() == false);
      auto &part = parts.back();
      part.second.append(reinterpret_cast<const char *>(data.data()), data.size());
   }

   auto on_part_end(error_code &ec) -> void
   {
      ++parts_count;
   }

   auto on_finish(error_code &ec) -> void
   {
      ++finish_count;
   }

   std::vector<std::pair<http::fields, std::string>> parts;
   std::size_t parts_count = 0;
   std::size_t finish_count = 0;
};

static_assert(multipart_event_handler<test_message_handler>,
              "test_message_handler should satisfy multipart_event_handler concept");

TEST_CASE("test simple multipart_parser", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   std::string_view data =
       "--boundary\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n\r\nHello "
       "World\r\n--boundary--\r\n";
   error_code ec;

   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.finish_count == 1);
   REQUIRE(handler.parts_count == 1);
   REQUIRE(handler.parts.size() == 1);
   REQUIRE(handler.parts[0].first["Content-Disposition"] == "form-data; name=\"file\"; filename=\"test.txt\"");
   REQUIRE(handler.parts[0].second == "Hello World\r\n");
}

TEST_CASE("test two parts multipart_parser", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   std::string_view data =
       "--boundary\r\n"
"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
"\r\n"
"Hello World"
"--boundary\r\n"
"Content-Disposition: form-data; name=\"file2\"; filename=\"test2.txt\"\r\n"
"\r\n"
"Hello Again\r\n"
"--boundary--\r\n";


   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.finish_count == 1);
   REQUIRE(handler.parts_count == 2);
   REQUIRE(handler.parts.size() == 2);
   REQUIRE(handler.parts[0].first["Content-Disposition"] == "form-data; name=\"file\"; filename=\"test.txt\"");
   REQUIRE(handler.parts[0].second == "Hello World");
   REQUIRE(handler.parts[1].first["Content-Disposition"] == "form-data; name=\"file2\"; filename=\"test2.txt\"");
   REQUIRE(handler.parts[1].second == "Hello Again\r\n");
}

TEST_CASE("test two parts multipart_parser - by single character", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   std::string_view data =
       "--boundary\r\n"
"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
"\r\n"
"Hello World"
"--boundary\r\n"
"Content-Disposition: form-data; name=\"file2\"; filename=\"test2.txt\"\r\n"
"\r\n"
"Hello Again\r\n"
"--boundary--\r\n";


   error_code ec;

   for (const char & i : data)
   {
      parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(&i), 1}, ec);
      if (ec)
      {
         break;
      }
   }
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.finish_count == 1);
   REQUIRE(handler.parts_count == 2);
   REQUIRE(handler.parts.size() == 2);
   REQUIRE(handler.parts[0].first["Content-Disposition"] == "form-data; name=\"file\"; filename=\"test.txt\"");
   REQUIRE(handler.parts[0].second == "Hello World");
   REQUIRE(handler.parts[1].first["Content-Disposition"] == "form-data; name=\"file2\"; filename=\"test2.txt\"");
   REQUIRE(handler.parts[1].second == "Hello Again\r\n");
}