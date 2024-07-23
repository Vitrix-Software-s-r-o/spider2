#pragma once

#include <spider2/routing.h>
#include <spider2/types.h>

namespace spider2::testing
{
   /**
    * @brief It executes one request using the given router using fresh io_context and a new thread.
    * It passes the request to the router pipeline and waits for its result.
    *
    * It solves problem of asynchrouns processing of the request in the unit tests.
    *
    * @param router The router to use
    * @param message The request to process
    * @return The response to the request
    */
   auto process_request(auto &router, http::request<http::string_body> message) -> response
   {
      static app_context_base ctx;
      auto req = request{std::move(message), ctx};

      auto executor = boost::asio::io_context{};

      auto result = co_spawn(executor, router(req), io::use_future);
      auto thread = std::jthread{[&] { executor.run(); }};

      return result.get();
   }
} // namespace spider2::testing