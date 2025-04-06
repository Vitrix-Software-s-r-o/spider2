//
// Created by jan on 6.4.25.
//

#pragma once

#include "any_message_body.h"

namespace spider2
{
   class multipart_message_body final : public any_message_reader
   {
    public:
      explicit multipart_message_body(boost::filesystem::path directory);

      void init(const http::fields &fields, const boost::optional<std::uint64_t> &length, error_code &ec);
      auto put(std::size_t allocation_hint_size, std::span<const std::byte>, error_code &ec) -> std::size_t;
      void finish(error_code &ec);
   };
} // namespace spider2