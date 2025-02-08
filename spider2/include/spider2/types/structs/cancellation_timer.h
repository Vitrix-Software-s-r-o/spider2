//
// Created by jan on 5.2.25.
//

#pragma once
#include "../platform.h"

namespace spider2
{
   class cancellation_timer
   {
    public:
      inline cancellation_timer(io::any_io_executor executor, stop_source &cts,
                                std::chrono::steady_clock::duration deadline)
          : cts(cts), deadline_(deadline), timer_(std::move(executor))
      {
      }

      inline auto start_watchdog(std::function<void()> callback = {})
      {
         timer_.expires_after(deadline_);
         timer_.async_wait(
             [this, callback](const error_code &ec)
             {
                if (!ec)
                {
                   if (callback != nullptr)
                   {
                      callback();
                   }
                   // timer finished and has not been cancelled -> we trigger cancellation
                   cts.request_stop();
                }
             });
         struct deadline_watchdog
         {
            deadline_watchdog(io::steady_timer &timer) : timer_(timer) {}
            ~deadline_watchdog()
            {
               stop();
            }

            void stop()
            {
               if (!stopped_)
               {
                  timer_.cancel();
               }
            }

          private:
            io::steady_timer &timer_;
            bool stopped_ = false;
         };

         return deadline_watchdog{timer_};
      }

    private:
      stop_source &cts;
      std::chrono::steady_clock::duration deadline_;
      io::steady_timer timer_;
   };
} // namespace spider2