// libFuzzer harness for spider2::multipart_message_parser — STREAMING variant.
//
// The single-shot harness (fuzz_multipart.cpp) feeds the whole body in one
// on_data() call, so the parser only ever processes its internal buffer from
// on_finish(). The capacity-triggered path — on_process_buffer() firing mid-read
// when buffer_ fills to max_part_header_size (8096), followed by buffer_to_front()
// re-buffering the unconsumed tail — is reached only when:
//   (a) data arrives in multiple on_data() calls (network reads), AND
//   (b) the total body exceeds the 8096-byte buffer so it actually fills.
// This harness exercises both. Run it with a large -max_len (e.g. 65536).
//
// Input format (chosen to be easy to hand-seed):
//   [1 byte : N            ] boundary length
//   [N bytes: boundary     ]
//   [1 byte : K (clamped 16)] number of chunk-size plan entries
//   [K bytes: plan         ] each byte b -> chunk size (b * 64) + 1  (1..16321)
//   [rest   : body         ] fed in chunks, cycling the plan
//
// The chunk plan lets the fuzzer steer where on_data() boundaries fall relative
// to multipart delimiters — the case where a token straddles two reads, which is
// exactly where iterator-advance bugs hide.

#include <spider2/types/utils/multipart_message_parser.h>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

using namespace spider2;

namespace
{
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
   if (size < 2)
      return 0;

   size_t pos = 0;
   const size_t boundary_len = data[pos++];
   if (pos + boundary_len > size)
      return 0;

   const std::string_view boundary{reinterpret_cast<const char *>(data + pos), boundary_len};
   pos += boundary_len;

   fuzz_handler handler;
   multipart_message_parser parser{boundary, handler};
   error_code ec;

   if (pos >= size)
   {
      parser.on_finish(ec);
      return 0;
   }

   // Chunk-size plan.
   const size_t plan_len = std::min<size_t>(data[pos++], 16);
   std::vector<size_t> plan;
   plan.reserve(plan_len);
   for (size_t i = 0; i < plan_len && pos < size; ++i)
      plan.push_back(static_cast<size_t>(data[pos++]) * 64 + 1);

   // Remaining bytes are the body; feed in chunks, cycling the plan.
   const auto *body = data + pos;
   const size_t body_size = size - pos;
   size_t off = 0;
   size_t plan_idx = 0;
   while (off < body_size && !ec)
   {
      const size_t chunk = plan.empty() ? 64 : plan[plan_idx++ % plan.size()];
      const size_t n = std::min(chunk, body_size - off);
      parser.on_data(std::span<const std::byte>{reinterpret_cast<const std::byte *>(body + off), n}, ec);
      off += n;
   }
   parser.on_finish(ec);

   return 0;
}
