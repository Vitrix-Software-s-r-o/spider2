#include <catch.hpp>
#include <spider2/types/utils/multipart_message_parser.h>

using namespace spider2;
struct test_message_handler
{
   constexpr auto on_part_begin(http::fields fields, error_code &ec) -> void {}

   constexpr auto on_part_data(std::span<const std::byte> data, error_code &ec) -> void {}

   constexpr auto on_part_end(error_code &ec) -> void {}
};

static_assert(multipart_event_handler<test_message_handler>,
              "test_message_handler should satisfy multipart_event_handler concept");

TEST_CASE("test multipart_parser", "[multipart_parser]")
{

   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"--boundary", handler};

   std::string_view data =
       "--boundary\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n\r\nHello "
       "World\r\n--boundary--\r\n";
   error_code ec;

   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   REQUIRE(!ec);
}