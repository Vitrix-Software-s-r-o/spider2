//
// Created by jhrub on 22.02.2023.
//

#pragma once

#include "../platform.h"

namespace spider2::async_file_io {

    /// it opens file and returns file_handle
    [[nodiscard]]
    auto open_binary_file(io::any_io_executor service, filesystem::path const &path) noexcept -> optional <file_handle>;

    /// it returns file size, on failure it returns -1 (max of uint64)
    [[nodiscard]]
    auto get_file_size(file_handle &handle) noexcept -> std::uint64_t;

    /**
     * it sends file over the TCP connection by the low-level OS API
     * it uses TransmitFile on Windows and sendfile on POSIX
     *
     * The file must have constant size while is read otherwise it will write only n-bytes available at the beginning.
     *
     * \returns awaitable tuple of error_code and bytes sent
     */
    [[nodiscard]]
    auto send_file(file_handle &handle, tcp::socket &socket,
                   std::uint64_t offset = 0,
                   std::uint64_t len = -1) noexcept -> io::awaitable<tuple<error_code, std::uint64_t>>;


    [[nodiscard]]
    /**
     * It marks file for deletion
     * Platform Windows:
     *      It marks the file for deletion. It stays in FS until is open. It gets deleted after close.
     * Platform Linux:
     *      It unlinks the file. It disappears from FS. It gets deleted after close.
     * @param handle
     * @return
     */
    auto delete_file_on_close(file_handle &handle) noexcept -> bool;


    [[nodiscard]]
    auto get_last_modified_time(file_handle &handle) noexcept -> optional <chrono::system_clock::time_point>;

    //TODO stream_file api might be convenient sometimes on linux for fifo files
    /*
    auto stream_file(file_handle const &handle, tcp::socket &socket,
                     std::uint64_t offset = 0,
                     std::uint64_t len = -1, std::stop_token stop_token) -> io::awaitable<tuple<error_code, size_t>>;
    */
}