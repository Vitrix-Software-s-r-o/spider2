#include "spider2/types/structs/app_context_base.h"
#include <fmt/format.h>
#include <iostream>

auto spider2::server_event_handler::on_io_thread_exception(std::exception const *ex) noexcept -> void
{
   std::cerr << fmt::format("spider2::app_context_base::on_server_error: {}", ex == nullptr ? "unknown" : ex->what())
             << std::endl;
}

auto spider2::server_event_handler::on_app_error(std::exception const *ex) noexcept -> void
{
   std::cerr << fmt::format("spider2::app_context_base::on_app_error: {}", ex == nullptr ? "unknown" : ex->what())
             << std::endl;
}

auto spider2::server_event_handler::on_connection_event(connection_event const &event) noexcept -> void
{
   const auto format_endpoint = [](auto const &ep)
   { return fmt::format("{}:{}", ep.address().to_string(), ep.port()); };
   switch (event.event_type)
   {
   case connection_event::event::listening_started:
      std::cout << fmt::format("spider2::app_context_base::on_connection_event: listening started on {}",
                               format_endpoint(event.accept_endpoint))
                << std::endl;
      break;
   case connection_event::event::listening_stopped:
      std::cout << fmt::format("spider2::app_context_base::on_connection_event: listening stopped on {}",
                               format_endpoint(event.accept_endpoint))
                << std::endl;
      break;
   case connection_event::event::accept_error:
      std::cerr << fmt::format("spider2::app_context_base::on_connection_event: accept error on {}: {}",
                               format_endpoint(event.accept_endpoint), event.error_message.value_or("unknown"))
                << std::endl;
      break;
   case connection_event::event::accepted_connection:
      if (event.client_endpoint.has_value())
      {
         std::cout << fmt::format("spider2::app_context_base::on_connection_event: accepted connection on {} from {}",
                                  format_endpoint(event.accept_endpoint),
                                  format_endpoint(event.client_endpoint.value()))
                   << std::endl;
      }
      else
      {
      }
      break;
   case connection_event::event::connection_closed:
      if (event.client_endpoint.has_value())
      {
         std::cout << fmt::format("spider2::app_context_base::on_connection_event: connection closed on {} from {}",
                                  format_endpoint(event.accept_endpoint),
                                  format_endpoint(event.client_endpoint.value()))
                   << std::endl;
      }
      break;
   default:
      break;
   }
}