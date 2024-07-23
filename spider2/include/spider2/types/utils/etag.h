//
// Created by jhrub on 04.03.2023.
//

#pragma once
#include "spider2/types/platform.h"

namespace spider2::etag
{
   string make_etag(chrono::system_clock::time_point last_write_time, std::uint64_t value) noexcept;
} // namespace spider2::etag