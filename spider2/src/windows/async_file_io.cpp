#include "spider2/types/utils/async_file_io.h"
#include "spider2/middleware/make_async_execution_handler.h"

using namespace boost::asio::windows;

namespace {

    bool is_handle_valid(HANDLE h) {
        const std::size_t invalid_handle = -1;
        return reinterpret_cast<std::size_t>(h) != invalid_handle;
    }
}
auto spider2::async_file_io::open_binary_file(io::any_io_executor service,
                                              filesystem::path const &path) noexcept -> optional <spider2::file_handle>
{
  HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
  if (is_handle_valid(h))
  {
      return {spider2::file_handle{service,  static_cast<file_handle::native_handle_type>(h)}};
  }

  return {};
}

auto spider2::async_file_io::get_file_size(file_handle &handle) noexcept -> std::uint64_t {
    LARGE_INTEGER result;
    memset(&result, 0, sizeof(LARGE_INTEGER));

    auto apiResult = GetFileSizeEx(handle.native_handle(), &result);
    if (apiResult == 0) return -1;
    return static_cast<std::uint64_t>(result.QuadPart);
}

namespace spider2::async_file_io
{
    namespace {
        constexpr std::size_t transmitfile_max = 2147483646;

        template<class Handler>
        class transmit_file_big_state : public std::enable_shared_from_this<transmit_file_big_state<Handler>> {
        public:
            transmit_file_big_state(tcp::socket &socket, file_handle &file, Handler&& handler,
                                    std::size_t offset, std::size_t len)
                    : socket_(socket), file_(file), handler_(std::forward<Handler>(handler)), offset_(offset), len_(len) {
            }

            void transmit_big_file() {
                auto self = this->shared_from_this();
                overlapped_ptr overlapped(socket_.get_executor(),
                                          [this, self](boost::system::error_code const &ec, std::size_t bytes) mutable {
                                              total_bytes_ += bytes;
                                              offset_ += bytes;
                                              len_ -= bytes;

                                              if (len_ == 0 || ec) {
                                                  handler_(ec, total_bytes_);
                                              } else {
                                                  transmit_big_file();
                                              }
                                          });

                LARGE_INTEGER offset;
                offset.QuadPart = static_cast<LONGLONG>(offset_);

                OVERLAPPED *overlapped_data = overlapped.get();
                overlapped_data->Offset = offset.LowPart;
                overlapped_data->OffsetHigh = offset.HighPart;

                auto len = std::min(len_, transmitfile_max);

                BOOL ok = ::TransmitFile(socket_.native_handle(), file_.native_handle(), static_cast<DWORD>(len), 0,
                                         overlapped.get(), 0, 0);
                DWORD last_error = ::GetLastError();

                // Check if the operation completed immediately.
                if (!ok && last_error != ERROR_IO_PENDING) {
                    // The operation completed immediately, so a completion notification needs
                    // to be posted. When complete() is called, ownership of the OVERLAPPED-
                    // derived object passes to the io_service.
                    boost::system::error_code ec(last_error, boost::asio::error::get_system_category());
                    overlapped.complete(ec, 0);
                } else {
                    // The operation was successfully initiated, so ownership of the
                    // OVERLAPPED-derived object has passed to the io_service.
                    overlapped.release();
                }
            }

        private:
            tcp::socket &socket_;
            file_handle &file_;
            Handler handler_;

            std::size_t offset_;
            std::size_t len_;
            std::size_t total_bytes_ = 0;
        };

        template<typename Handler>
        void transmit_file(boost::asio::ip::tcp::socket &socket, file_handle &file, std::size_t offset_in,
                           std::size_t len, Handler && handler) {
            if (len <= transmitfile_max) {
                using namespace boost::asio::windows;

                // Construct an OVERLAPPED-derived object to contain the handler.
                overlapped_ptr overlapped(socket.get_executor(), std::forward<Handler>(handler));

                LARGE_INTEGER offset;
                offset.QuadPart = static_cast<LONGLONG>(offset_in);

                OVERLAPPED *overlapped_data = overlapped.get();
                overlapped_data->Offset = offset.LowPart;
                overlapped_data->OffsetHigh = offset.HighPart;

                // Initiate the TransmitFile operation.
                BOOL ok = ::TransmitFile(socket.native_handle(), file.native_handle(), static_cast<DWORD>(len), 0,
                                         overlapped.get(), 0, 0);
                DWORD last_error = ::GetLastError();

                // Check if the operation completed immediately.
                if (!ok && last_error != ERROR_IO_PENDING) {
                    // The operation completed immediately, so a completion notification needs
                    // to be posted. When complete() is called, ownership of the OVERLAPPED-
                    // derived object passes to the io_service.
                    boost::system::error_code ec(last_error, boost::asio::error::get_system_category());
                    overlapped.complete(ec, 0);
                } else {
                    // The operation was successfully initiated, so ownership of the
                    // OVERLAPPED-derived object has passed to the io_service.
                    overlapped.release();
                }
            } else {
                auto transmit_state =
                        std::make_shared<transmit_file_big_state<Handler>>(socket, file, std::forward<Handler>(handler), offset_in, len);
                transmit_state->transmit_big_file();
            }
        }
    }
}

auto spider2::async_file_io::send_file(file_handle &handle, tcp::socket &socket,
                                       std::uint64_t offset,
                                       std::uint64_t len) noexcept -> io::awaitable <tuple<error_code, std::uint64_t>>

{
    if (len == std::numeric_limits<std::uint64_t>::max())
    {
        len = get_file_size(handle);
        if (offset < len)
        {
          len = len - offset;
        }
        else
        {
            len = 0;
        }
    }
    auto comletion_token = ioe::as_tuple(io::use_awaitable);
    return io::async_initiate<decltype(comletion_token),
        void(error_code, std::uint64_t)>([&socket, &handle, offset, len](auto &&handler) {
            transmit_file(socket, handle, offset, len, std::forward<std::decay_t<decltype(handler)>>(handler));
        }, comletion_token);
}

auto spider2::async_file_io::delete_file_on_close(file_handle &handle) noexcept -> bool
{
    FILE_DISPOSITION_INFO fdi;
    memset(&fdi, 0, sizeof(FILE_DISPOSITION_INFO));

    fdi.DeleteFile = TRUE; // marking for deletion

    BOOL fResult = SetFileInformationByHandle( handle.native_handle(),
                                               FileDispositionInfo,
                                               &fdi,
                                               sizeof(FILE_DISPOSITION_INFO));


    return fResult != FALSE;
}

auto spider2::async_file_io::get_last_modified_time(file_handle &handle) noexcept
    -> optional<chrono::system_clock::time_point>
{
    FILETIME last_write_time{};
    if (GetFileTime(
        handle.native_handle(),
        NULL,
        NULL,
        &last_write_time
    ) == TRUE)
    {

      auto result = chrono::file_clock::duration {(static_cast<std::int64_t>(last_write_time.dwHighDateTime) << 32)
                               | last_write_time.dwLowDateTime};
      return chrono::clock_cast<chrono::system_clock>(chrono::file_clock::time_point{result});
    }

    return {};
}
