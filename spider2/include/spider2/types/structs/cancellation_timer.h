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
      cancellation_timer(stop_source &cts, std::chrono::steady_clock::duration deadline) : cts(cts) {}

      auto make_deadline_watchdog()
      {
         timer_.expires_after(deadline_);
         struct deadline_watchdog
         {
            ~deadline_watchdog()
            {
               timer_.cancel();
            }
         };
      }

    private:
      stop_source &cts;
      io::steady_timer timer_;
      std::chrono::steady_clock::duration deadline_;
   };
} // namespace spider2