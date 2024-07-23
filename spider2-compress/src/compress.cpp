//
// Created by jhrub on 5/10/2023.
//
#include <brotli/decode.h>
#include <brotli/encode.h>

#include "spider2/middleware/compress.h"

auto spider2::brotli::compress(string_view buffer, int quality) -> string
{
   if (buffer.empty()) return {};

   size_t allocate_bytes = BrotliEncoderMaxCompressedSize(buffer.size());

   auto output_buffer = string{};
   output_buffer.resize(allocate_bytes);

   if (!BrotliEncoderCompress(quality, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, static_cast<size_t>(buffer.size()),
                              reinterpret_cast<const uint8_t *>(buffer.data()), &allocate_bytes,
                              reinterpret_cast<uint8_t *>(output_buffer.data())))
   {
      return {};
   }
   else
   {
      output_buffer.resize(allocate_bytes);
      return output_buffer;
   }
}

auto spider2::brotli::decompress(string_view buffer) -> string
{
   if (buffer.empty()) return {};

   auto decompress_buffer = string{};
   decompress_buffer.resize(buffer.size() * 10);

   auto buffer_size = decompress_buffer.size();

   BrotliDecoderResult result = BrotliDecoderResult::BROTLI_DECODER_RESULT_SUCCESS;

   result =
       BrotliDecoderDecompress(static_cast<size_t>(buffer.size()), reinterpret_cast<const uint8_t *>(buffer.data()),
                               &buffer_size, reinterpret_cast<uint8_t *>(decompress_buffer.data()));

   int i = 10;
   while (result == BrotliDecoderResult::BROTLI_DECODER_RESULT_ERROR && i-- > 0)
   {
      decompress_buffer.resize(buffer_size * 2);
      buffer_size = decompress_buffer.size();

      result =
          BrotliDecoderDecompress(static_cast<size_t>(buffer.size()), reinterpret_cast<const uint8_t *>(buffer.data()),
                                  &buffer_size, reinterpret_cast<uint8_t *>(decompress_buffer.data()));
   }
   decompress_buffer.resize(buffer_size);

   return decompress_buffer;
}
