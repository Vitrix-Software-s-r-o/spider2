#include "spider2/types/structs/app_context_base.h"
#include <fmt/format.h>
#include <iostream>
auto spider2::server_event_handler::on_io_thread_exception(std::exception const *ex) noexcept -> void
{
   std::cerr << fmt::format("spider2::app_context_base::on_server_error: {}", ex == nullptr ? "unknown" : ex->what())
             << std::endl;
}