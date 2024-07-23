//
// Created by jhrub on 18.12.2022.
//

#pragma once

#include <condition_variable>
#include <mutex>
#include <stop_token>

namespace spider2
{
   class start_indicator
   {
    public:
      inline void set_configured_threads(int threads)
      {
         configured_threads = threads;
      }

      inline void thread_started()
      {
         {
            auto lock = std::lock_guard{m_};
            ++running_threads;
         }
         cv_.notify_all();
      }

      inline auto wait_for_all_threads_started() const -> bool
      {
         auto lock = std::unique_lock{m_};
         cv_.wait(lock, [&] { return running_threads == configured_threads; });
         return true;
      }

      inline auto wait_for_all_threads_started(std::stop_token token) const -> bool
      {
         auto lock = std::unique_lock{m_};
         cv_.wait(lock, [&] { return running_threads == configured_threads || token.stop_requested(); });

         return running_threads == configured_threads;
      }

    private:
      mutable std::mutex m_;
      mutable std::condition_variable cv_;
      int running_threads = 0;
      int configured_threads = -1;
   };

   class server_event_handler
   {
    public:
      virtual ~server_event_handler() = default;
      virtual auto on_io_thread_exception(std::exception const *ex) noexcept -> void;
   };

   struct app_context_base
   {
      std::stop_token token;
      start_indicator lifecycle_indicator;

      server_event_handler *event_handler = nullptr;
   };
} // namespace spider2