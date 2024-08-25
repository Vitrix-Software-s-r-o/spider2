#include "spider2/types/utils/ignore_sig_pipe.h"
#include <assert.h>
#include <cstring>
#include <signal.h>
#include <unistd.h>

void spider2::ignore_sig_pipe() noexcept
{
   struct sigaction sa;
   std::memset(&sa, 0, sizeof(sa));
   sa.sa_handler = SIG_IGN;
   static_cast<void>(sigaction(SIGPIPE, &sa, NULL));
}