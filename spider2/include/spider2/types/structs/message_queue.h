//
// Created by jan on 5.2.25.
//

#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <variant>

#include "../platform.h"

namespace spider2
{
   /// @brief A message that can be sent to a client. std::shared_ptr<string> is used to allow for multicast messages.
   using outgoing_message = std::variant<string, std::shared_ptr<string>>;

   /// @brief A message queue for outgoing messages.
   template <typename T>
   using is_outgoing_message = std::is_constructible<outgoing_message, T>;

   /// @brief A concept for outgoing messages.
   template <typename T>
   concept outgoing_message_concept = is_outgoing_message<T>::value;

   /// @brief A message queue for outgoing messages.
   /// @details This class is thread-safe. It uses a mutex to protect the queue.
   class message_queue
   {
    public:
      inline explicit message_queue(io::any_io_executor executor) : timer_(std::move(executor)) {}

      /// @brief Queue a message.

      inline void queue(outgoing_message_concept auto &&msg)
      {
         std::lock_guard<std::mutex> lock(lock_);
         queue_.push(std::forward<std::remove_reference_t<decltype(msg)>>(msg));
      }

      /// @brief Dequeue a message.
      /// @return The message or std::nullopt if the queue is empty.
      inline auto dequeue() -> std::optional<outgoing_message>
      {
         std::lock_guard<std::mutex> lock(lock_);
         if (queue_.empty())
         {
            return std::optional<outgoing_message>{};
         }

         auto msg = queue_.front();
         queue_.pop();

         return msg;
      }

      [[nodiscard]]
      inline auto async_dequeue(std::stop_token const &token) -> io::awaitable<std::optional<outgoing_message>>
      {
         timer_.expires_after(60s);
         const auto watchdog = std::stop_callback{token, [&]() { timer_.cancel(); }};
         while (!token.stop_requested())
         {
            if (const auto [timer_ec] = co_await timer_.async_wait(use_tuple_awaitable); timer_ec)
            {
               if (token.stop_requested())
               {
                  co_return std::nullopt;
               }
               else
               {
                  co_return dequeue();
               }
            }
         }
      }

    private:
      mutable std::mutex lock_;
      std::queue<outgoing_message> queue_;
      boost::asio::steady_timer timer_;
   };
} // namespace spider2