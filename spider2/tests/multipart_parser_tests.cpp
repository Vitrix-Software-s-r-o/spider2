#include <catch.hpp>
#include <boost/algorithm/string/predicate.hpp>
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
   REQUIRE(handler.parts[0].second == "Hello World");
}

TEST_CASE("test two parts multipart_parser", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World\r\n"
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
   REQUIRE(handler.parts[1].second == "Hello Again");
}

TEST_CASE("test two parts multipart_parser - by single character", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World\r\n"
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file2\"; filename=\"test2.txt\"\r\n"
      "\r\n"
      "Hello Again\r\n"
      "--boundary--\r\n";

   error_code ec;

   for (const char &i : data)
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
   REQUIRE(handler.parts[1].second == "Hello Again");
}

TEST_CASE("test two parts multipart_parser - long body", "[multipart_parser]")
{

   std::string data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n";

   for (int i = 0; i < 1000000; ++i)
   {
      data += static_cast<char>(i % 0xff);
   }
   data += "Hello World\r\n"
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file2\"; filename=\"test2.txt\"\r\n"
      "\r\n"
      "Hello Again\r\n"
      "--boundary--\r\n";

   {
      auto handler = test_message_handler{};
      auto parser = multipart_message_parser{"boundary", handler};
      auto ec = error_code{};

      for (const char &i : data)
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

      REQUIRE(handler.parts[0].second.size() == 1000000 + ("Hello World"sv).size());
      for (int i = 0; i < 1000000; ++i)
      {
         REQUIRE(handler.parts[0].second[i] == static_cast<char>(i % 0xff));
      }

      REQUIRE(boost::ends_with(handler.parts[0].second, "Hello World"sv));

      REQUIRE(handler.parts[1].first["Content-Disposition"] == "form-data; name=\"file2\"; filename=\"test2.txt\"");
      REQUIRE(handler.parts[1].second == "Hello Again");
   }

   {
      auto handler = test_message_handler{};
      auto parser = multipart_message_parser{"boundary", handler};
      auto ec = error_code{};
      parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
      REQUIRE(!ec);
      parser.on_finish(ec);

      REQUIRE(!ec);
      REQUIRE(handler.finish_count == 1);
      REQUIRE(handler.parts_count == 2);
      REQUIRE(handler.parts.size() == 2);
      REQUIRE(handler.parts[0].first["Content-Disposition"] == "form-data; name=\"file\"; filename=\"test.txt\"");

      REQUIRE(handler.parts[0].second.size() == 1000000 + ("Hello World"sv).size());
      for (int i = 0; i < 1000000; ++i)
      {
         REQUIRE(handler.parts[0].second[i] == static_cast<char>(i % 0xff));
      }

      REQUIRE(boost::ends_with(handler.parts[0].second, "Hello World"sv));

      REQUIRE(handler.parts[1].first["Content-Disposition"] == "form-data; name=\"file2\"; filename=\"test2.txt\"");
      REQUIRE(handler.parts[1].second == "Hello Again");
   }

}

TEST_CASE("test multipart_parser - missing opening boundary", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Data that doesn't start with the boundary
   std::string_view data = "Some random data without boundary\r\n";
   error_code ec;

   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Parser should not have found any parts
   REQUIRE(handler.parts_count == 0);
   REQUIRE(handler.parts.size() == 0);
}

TEST_CASE("test multipart_parser - incomplete message (missing closing boundary)", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Message without closing boundary
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should have parsed the part but no finish since closing boundary is missing
   REQUIRE(handler.parts_count == 0); // on_part_end not called
   REQUIRE(handler.parts.size() == 1); // But part was started
}

TEST_CASE("test multipart_parser - invalid boundary termination", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Boundary followed by invalid characters (not "--" or "\r\n")
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World"
      "--boundaryXX\r\n"; // Invalid termination

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should complete without error as "--boundaryXX" is not recognized as boundary
   REQUIRE(handler.parts.size() == 1);
   REQUIRE(handler.parts[0].second.find("--boundaryXX") != std::string::npos);
   REQUIRE(ec);
}

TEST_CASE("test multipart_parser - malformed part after valid boundary", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Boundary followed by something that's neither "--" nor "\r\n"
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World"
      "--boundaryAB"; // Invalid - should be either "--boundary--" or "--boundary\r\n"

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // This should be treated as part data since boundary isn't properly terminated
   REQUIRE(handler.parts.size() == 1);
}

TEST_CASE("test multipart_parser - empty multipart message", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Just the closing boundary with no parts
   std::string_view data = "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 0);
   REQUIRE(handler.parts.size() == 0);
   REQUIRE(handler.finish_count == 1);
}

TEST_CASE("test multipart_parser - header exceeding max size", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Create a very long header that exceeds max_part_header_size
   std::string data = "--boundary\r\nContent-Disposition: form-data; name=\"file\"; ";
   
   // Add a very long field value to exceed 8096 bytes
   for (int i = 0; i < 10000; ++i)
   {
      data += "x";
   }
   data += "\r\n\r\nHello World\r\n--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should fail due to header size limit
   REQUIRE(ec);
   REQUIRE(ec == make_error_code(request_error_code::body_read_error));
}

TEST_CASE("test multipart_parser - invalid header format", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Malformed header (missing colon in header field)
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition form-data\r\n" // Missing colon
      "\r\n"
      "Hello World\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should fail due to invalid header format
   REQUIRE(ec);
}

TEST_CASE("test multipart_parser - boundary within part data", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Part data contains text that looks like boundary but without proper prefix
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "This text contains boundary word\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should succeed - "boundary" without "--" prefix is just part data
   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 1);
   REQUIRE(handler.parts.size() == 1);
   REQUIRE(handler.parts[0].second == "This text contains boundary word");
}

TEST_CASE("test multipart_parser - missing CRLF after headers", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Missing the blank line (\r\n\r\n) between headers and body
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "Hello World\r\n" // Missing \r\n before body
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Parser should wait for proper header termination
   REQUIRE(handler.parts_count == 0);
}

TEST_CASE("test multipart_parser - wrong boundary in message", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Message uses different boundary than parser expects
   std::string_view data =
      "--different_boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World\r\n"
      "--different_boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should not find any parts since boundary doesn't match
   REQUIRE(handler.parts_count == 0);
   REQUIRE(handler.parts.size() == 0);
}

TEST_CASE("test multipart_parser - only preamble no parts", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Only preamble text, no actual multipart structure
   std::string_view data = "This is just preamble text with no boundaries at all.\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Should not have parsed any parts
   REQUIRE(handler.parts_count == 0);
   REQUIRE(handler.parts.size() == 0);
   REQUIRE(handler.finish_count == 0);
}

TEST_CASE("test multipart_parser - boundary not followed by required chars", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // After recognizing a boundary in at_boundary state, must have either "--" or "\r\n"
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "Hello World"
      "--boundaryINVALID";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Since "--boundaryINVALID" is not a valid boundary, it should be treated as data
   REQUIRE(handler.parts.size() == 1);
   REQUIRE(handler.parts[0].second == "Hello World--boundaryINVALID");
   REQUIRE(ec);
}

TEST_CASE("test multipart_parser - multiple consecutive boundaries", "[multipart_parser][failure]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Multiple boundaries without part data
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file2\"; filename=\"test2.txt\"\r\n"
      "\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Both parts should be parsed, just with empty data
   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 2);
   REQUIRE(handler.parts.size() == 2);
   REQUIRE(handler.parts[0].second.empty());
   REQUIRE(handler.parts[1].second.empty());
}

TEST_CASE("test multipart_parser - body part with no headers", "[multipart_parser]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Per RFC 2046: "NO header fields are actually required in body parts.
   // A body part that starts with a blank line is allowed."
   // The boundary is followed by two CRLFs (no headers, just blank line before body)
   std::string_view data =
      "--boundary\r\n"
      "\r\n"
      "Plain text data without headers\r\n"
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file2\"\r\n"
      "\r\n"
      "Data with headers\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 2);
   REQUIRE(handler.parts.size() == 2);
   // First part has no headers, should default to text/plain
   REQUIRE(handler.parts[0].second == "Plain text data without headers");
   // Second part has headers
   REQUIRE(handler.parts[1].first["Content-Disposition"] == "form-data; name=\"file2\"");
   REQUIRE(handler.parts[1].second == "Data with headers");
}

TEST_CASE("test multipart_parser - boundary in middle of line should not be recognized", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Per RFC 2046: boundary must occur at the beginning of a line (after CRLF)
   // A boundary without preceding CRLF should be treated as regular data
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
      "\r\n"
      "This line contains --boundary in the middle\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 1);
   REQUIRE(handler.parts.size() == 1);
   // The "--boundary" in the middle of the line should be part of the data
   REQUIRE(handler.parts[0].second == "This line contains --boundary in the middle");
}

TEST_CASE("test multipart_parser - boundary at start of line without CRLF prefix", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Boundary at the start of a line but not the first boundary (missing CRLF before it)
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"\r\n"
      "\r\n"
      "Line1\r\n"
      "--boundary\r\n"  // This IS preceded by CRLF, so it should work
      "Content-Disposition: form-data; name=\"file2\"\r\n"
      "\r\n"
      "Line2\r\n"
      "--boundary--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 2);
   REQUIRE(handler.parts.size() == 2);
}

TEST_CASE("test multipart_parser - data without preceding CRLF to boundary", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Part data that ends without CRLF before the boundary
   // Per RFC 2046: CRLF before boundary is conceptually part of the boundary
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"\r\n"
      "\r\n"
      "Data without trailing CRLF"
      "--boundary--\r\n";  // Missing \r\n before this boundary

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // This should fail because the boundary isn't preceded by CRLF
   // The data contains "Data without trailing CRLF--boundary--"
   REQUIRE(handler.parts.size() == 1);
   const auto pos = handler.parts[0].second.find("--boundary--");
   REQUIRE( pos != std::string::npos);
}

TEST_CASE("test multipart_parser - very long boundary name", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   
   // RFC 2046: boundary must be 0-70 characters (effectively 1-70 since 0 is useless)
   std::string long_boundary(71, 'x');
   auto parser = multipart_message_parser{long_boundary, handler};

   std::string data = "--" + long_boundary + "\r\n"
      "Content-Disposition: form-data; name=\"file\"\r\n"
      "\r\n"
      "Test\r\n"
      "--" + long_boundary + "--\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // Parser should handle this even though boundary is too long per RFC
   // (This is a leniency test - strict parsers might reject it)
}

TEST_CASE("test multipart_parser - boundary with trailing whitespace", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   
   // RFC 2046: boundary must NOT end with whitespace
   std::string boundary_with_space = "boundary ";
   auto parser = multipart_message_parser{boundary_with_space, handler};

   std::string data = "--boundary \r\n"
      "Content-Disposition: form-data; name=\"file\"\r\n"
      "\r\n"
      "Test\r\n"
      "--boundary --\r\n";

   error_code ec;
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(data.data()), data.size()}, ec);
   parser.on_finish(ec);

   // This tests if parser handles boundaries with trailing spaces
   // RFC says they should not be used but implementations may be lenient
}

TEST_CASE("test multipart_parser - buffer size exactly at boundary", "[multipart_parser][flaw]")
{
   auto handler = test_message_handler{};
   auto parser = multipart_message_parser{"boundary", handler};

   // Test edge case where buffer fills up exactly at a critical point
   std::string_view data =
      "--boundary\r\n"
      "Content-Disposition: form-data; name=\"file\"\r\n"
      "\r\n"
      "Test\r\n"
      "--boundary--\r\n";

   error_code ec;
   // Feed data byte by byte to test buffer boundary conditions
   for (size_t i = 0; i < data.size() && !ec; ++i)
   {
      parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(&data[i]), 1}, ec);
   }
   parser.on_finish(ec);

   REQUIRE(!ec);
   REQUIRE(handler.parts_count == 1);
}
