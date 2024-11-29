//
// Created by jhrub on 22.02.2023.
//
#include "spider2/types/utils/async_file_io.h"
#include <fmt/format.h>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace spider2::async_file_io
{
   namespace
   {
      template <class Handler>
      struct stream_op : std::enable_shared_from_this<stream_op<Handler>>
      {
         stream_op(boost::asio::ip::tcp::socket &socket, int fd, off_t start, std::int64_t len, Handler &&callback)
             : socket_(socket), fd_(fd), start_(start), count_(len), callback_(std::forward<Handler>(callback))
         {
         }

         void start()
         {
            if (count_ == 0)
            {
               this->callback_(spider2::error_code{}, std::uint64_t{});
            }
            else
            {
               ssize_t result = ::sendfile(socket_.native_handle(), fd_, &start_, count_);
               if (result == -1)
               {
                  auto code = boost::system::errc::make_error_code(static_cast<boost::system::errc::errc_t>(errno));
                  this->callback_(code, count_);
               }
               else if (result == 0)
               {
                  this->callback_(spider2::error_code{}, std::uint64_t{});
               }
               else
               {

                  if (result > count_)
                  {
                     count_ = 0;
                  }
                  else
                  {
                     count_ -= result;
                  }
                  // this will do nothing besides checking if socket
                  // is writable again.
                  auto self = this->shared_from_this();
                  socket_.async_write_some(
                      boost::asio::null_buffers(),
                      [this, self](const boost::system::error_code &ec, std::size_t bytes_transferred)
                      {
                         if (ec)
                         {
                            this->callback_(ec, bytes_transferred);
                         }
                         else
                         {
                            start();
                         }
                      });
               }
            }
         }

         tcp::socket &socket_;
         int fd_;
         off_t start_;
         std::int64_t count_;

         Handler callback_;
      };
   } // namespace

   constexpr bool is_handle_valid(int h)
   {
      return h >= 0;
   }
} // namespace spider2::async_file_io

auto spider2::async_file_io::open_binary_file(io::any_io_executor service, filesystem::path const &path) noexcept
    -> optional<spider2::file_handle>
{
   auto h = open(path.c_str(), O_RDONLY);
   if (is_handle_valid(h))
      return {file_handle{service, h}};
   else
      return {};
}

auto spider2::async_file_io::get_file_size(file_handle &handle) noexcept -> std::uint64_t
{
   struct stat buf
   {
   };
   if (fstat(handle.native_handle(), &buf) < 0)
   {
      return static_cast<std::uint64_t>(-1);
   }

   return static_cast<std::size_t>(buf.st_size);
}

auto spider2::async_file_io::send_file(file_handle &handle, tcp::socket &socket, std::uint64_t offset,
                                       std::uint64_t len) noexcept -> io::awaitable<tuple<error_code, std::uint64_t>>
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
   return io::async_initiate<decltype(comletion_token), void(error_code, std::uint64_t)>(
       [&socket, &handle, offset, len](auto &&handler)
       {
          auto async_operation = std::make_shared<stream_op<std::decay_t<decltype(handler)>>>(
              socket, handle.native_handle(), gsl::narrow_cast<off_t>(offset), len,
              std::forward<std::decay_t<decltype(handler)>>(handler));
          async_operation->start();
       },
       comletion_token);
}

auto spider2::async_file_io::delete_file_on_close(file_handle &handle) noexcept -> bool
{
   char file_path[PATH_MAX + 1]{};

   bool result = false;
#ifdef F_GETPATH
   result = fcntl(handle.native_handle(), F_GETPATH, file_path) != -1;
#else
   auto native_handle = handle.native_handle();
   auto read_link_path = fmt::format("/proc/self/fd/{}", native_handle);

   auto read_link_result = readlink(read_link_path.c_str(), file_path, PATH_MAX);
   if (read_link_result <= 0)
   {
      result = false;
   }
   else
   {
      file_path[read_link_result] = 0;
      result = true;
   }
#endif

   if (result)
   {
      result = unlink(file_path) != -1;
   }

   return result;
}

auto spider2::async_file_io::get_last_modified_time(file_handle &handle) noexcept
    -> spider2::optional<chrono::system_clock::time_point>
{
   struct stat stat_data
   {
   };

   if (fstat(handle.native_handle(), &stat_data) != -1)
   {
      return {chrono::system_clock::from_time_t(stat_data.st_mtim.tv_sec)};
   }

   return {};
}
