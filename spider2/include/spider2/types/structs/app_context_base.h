//
// Created by jhrub on 18.12.2022.
//

#pragma once

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <stop_token>

namespace spider2
{
   class start_indicator
   {
    public:
      inline void set_configured_threads(int threads)
      {
         shared_state_->set_configured_threads(threads);
      }

      inline void thread_started()
      {
         shared_state_->thread_started();
      }

      inline auto wait_for_all_threads_started() const -> bool
      {
         return shared_state_->wait_for_all_threads_started();
      }

      inline auto wait_for_all_threads_started(std::stop_token token) const -> bool
      {
         return shared_state_->wait_for_all_threads_started(token);
      }

    private:
      struct shared_state
      {
         mutable std::mutex m_;
         mutable std::condition_variable cv_;
         int running_threads = 0;
         int configured_threads = -1;

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
      };
      std::shared_ptr<shared_state> shared_state_ = std::make_shared<shared_state>();
   };

   class server_event_handler
   {
    public:
      struct connection_event
      {
         enum class event
         {
            listening_started,
            listening_stopped,
            accept_error,
            accepted_connection,
            connection_closed
         };
         event event_type;
         boost::asio::ip::tcp::endpoint accept_endpoint;
         std::optional<std::string> error_message = std::nullopt;
         std::optional<boost::asio::ip::tcp::endpoint> client_endpoint = std::nullopt;
      };
      virtual ~server_event_handler() = default;
      virtual auto on_io_thread_exception(std::exception const *ex) noexcept -> void;
      virtual auto on_app_error(std::exception const *ex) noexcept -> void;

      virtual auto on_connection_event(connection_event const &event) noexcept -> void;
   };

   struct app_context_base
   {
      std::stop_token token;
      start_indicator lifecycle_indicator = {};

      server_event_handler *event_handler = nullptr;
   };
} // namespace spider2