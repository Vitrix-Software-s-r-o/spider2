//
// Created by jhrub on 18.01.2023.
//

#pragma once

#include "asio.h"
#include "handlers.h"
#include "routing.h"
#include "server.h"
#include "types.h"

#include "middleware/exception_handling_middleware.h"
#include <ranges>

namespace spider2
{

   inline void run_io_context(io::io_context &io_loop, server_config const &config, app_context_base &context)
   {
      context.lifecycle_indicator.thread_started();
      bool has_run = false;
      while (!context.token.stop_requested())
      {
         try
         {
            if (has_run)
            {
               io_loop.restart();
            }
            else
            {
               has_run = true;
            }

            io_loop.run();
         }
         catch (const std::exception &ex)
         {
            if (context.event_handler != nullptr)
               context.event_handler->on_io_thread_exception(&ex);
            else
               std::cerr << "spider2::io_thread_exception: " << ex.what() << std::endl;
         }
         catch (...)
         {
            if (context.event_handler != nullptr)
               context.event_handler->on_io_thread_exception(nullptr);
            else
               std::cerr << "spider2::io_thread_exception: unknown" << std::endl;
         }
      }
   }

   auto run_web_app(io::io_context &io_loop, server_config const &config, app_context_base &context,
                    auto request_processor_fun) -> void
   {
      ignore_sig_pipe();

      const auto get_thread_count = [](std::uint8_t threads_num) -> std::size_t
      {
         if (threads_num == 0xff || threads_num == 0)
         {
            return std::thread::hardware_concurrency();
         }
         else
         {
            return threads_num;
         }
      };

      const auto block_run = [&]()
      {
         auto stop_callback = make_stop_callback(context.token, [&io_loop] { io_loop.stop(); });
         auto io_threads_count = get_thread_count(config.io_threads);
         context.lifecycle_indicator.set_configured_threads(gsl::narrow<int>(io_threads_count));

         if (io_threads_count == 1)
         {
            run_io_context(io_loop, config, context);
         }
         else
         {
            vector<std::jthread> threads;
            for (std::size_t i = 0; i != io_threads_count; ++i)
            {
               threads.push_back(std::jthread{[=, &io_loop, &context] { run_io_context(io_loop, config, context); }});
            }

            for (auto &thread : threads)
            {
               thread.join();
            }
         }
      };

      const auto tcp_listen = [&](auto &handler_fun)
      {
         for (auto &endpoint : config.tcp_listen)
         {
            io::co_spawn(io_loop, serve(context, config, endpoint, handler_fun), io::detached);
         }
      };
      if (config.thread_pool_threads > 0)
      {
         thread_pool proc_pool{get_thread_count(config.thread_pool_threads)};
         exception_handling_middleware thread_pool_handler{proc_pool, std::move(request_processor_fun)};
         auto iot_count = get_thread_count(config.io_threads);
         for (size_t t = 0; t != iot_count; ++t)
         {
            tcp_listen(thread_pool_handler);
         }

         block_run();
      }
      else
      {
         exception_handling_middleware catch_all_handler{io_loop, std::move(request_processor_fun)};
         tcp_listen(catch_all_handler);
         block_run();
      }
   }

} // namespace spider2