//
// Created by jhrub on 18.01.2023.
//

#pragma once

#include <cstdint>
#include "../platform.h"

namespace spider2 {
    struct server_config {
        vector<tcp::endpoint> tcp_listen;

        /// if set to 0 the thread pool is not created, if set to 0xff it will get selected by number of logical cores
        std::uint8_t thread_pool_threads = 0xff;

        /// if set to 0 and 0xff it will get selected by number of logical cores
        std::uint8_t io_threads = 0xff;

        /// how long is going to be connection alive after request is processed
        std::chrono::steady_clock::duration connection_keep_alive = std::chrono::minutes{2};
    };

}