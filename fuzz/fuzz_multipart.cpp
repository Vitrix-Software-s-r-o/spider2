// libFuzzer harness for spider2::multipart_message_parser.
//
// Build (clang, header-only — no need to build the rest of spider2):
//   see fuzz/build.sh
//
// The fuzzer input is split as:
//   [1 byte: boundary length N][N bytes: boundary][rest: multipart body]
// This lets the fuzzer explore both the boundary and the body, which is
// important because the parser's behaviour is driven by the boundary value.

#include <spider2/types/utils/multipart_message_parser.h>

#include <cstdint>
#include <span>
#include <string_view>

using namespace spider2;

namespace
{
   // Minimal handler that touches every callback argument so the optimizer
   // cannot discard the work and ASan/UBSan see real reads of the data.
   struct fuzz_handler
   {
      void on_part_begin(http::fields fields, error_code & /*ec*/)
      {
         for (auto const &f : fields)
            sink ^= f.value().size();
      }

      void on_part_data(std::span<const std::byte> data, error_code & /*ec*/)
      {
         for (auto b : data)
            sink ^= static_cast<unsigned>(b);
      }

      void on_part_end(error_code & /*ec*/) { ++sink; }
      void on_finish(error_code & /*ec*/) { ++sink; }

      unsigned sink = 0;
   };
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   if (size < 1)
      return 0;

   const size_t boundary_len = data[0];
   if (boundary_len + 1 > size)
      return 0;

   const std::string_view boundary{reinterpret_cast<const char *>(data + 1), boundary_len};
   const auto *body = data + 1 + boundary_len;
   const size_t body_size = size - 1 - boundary_len;

   fuzz_handler handler;
   multipart_message_parser parser{boundary, handler};

   error_code ec;
   // Feed the whole body in one shot. (A second harness could split the body
   // at a fuzzer-chosen offset to exercise the streaming / re-buffering path.)
   parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(body), body_size}, ec);
   parser.on_finish(ec);

   return 0;
}
