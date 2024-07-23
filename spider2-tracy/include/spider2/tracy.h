#pragma once
#include <format>
#include <spider2/types.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyC.h>

namespace spider2
{
   struct tracy_context
   {
      TracyCZoneCtx zone;
      std::string const &fiber_id;

      http::status status = http::status::ok;
      struct scope
      {
         tracy_context &ctx;
         inline explicit scope(tracy_context &ctx) : ctx(ctx)
         {
            ctx.enter();
         }

         inline ~scope()
         {
            ctx.suspend();
         }

         inline auto zone() -> TracyCZoneCtx &
         {
            return ctx.zone;
         }
      };

      inline auto enter_scope() -> auto
      {
         return scope(*this);
      }

      inline explicit tracy_context(std::string const &fiber_id_) : fiber_id(fiber_id_)
      {
         auto scope = enter_scope();
         TracyCZone(ctx, true);
         TracyMessage(fiber_id.c_str(), fiber_id.size());
         zone = ctx;
      }

      inline tracy_context(tracy_context const &o) = delete;
      inline tracy_context(tracy_context &&o) = delete;

      ~tracy_context() noexcept(false)
      {
         auto scope = enter_scope();
         auto message = std::format("{} -> {}", fiber_id, static_cast<unsigned>(status));
         TracyMessage(message.c_str(), message.size());
         TracyCZoneEnd(zone);
      }
      inline auto suspend() const -> void
      {
         TracyFiberLeave;
      }

      inline auto enter() const -> void
      {
         TracyFiberEnter(fiber_id.c_str());
      }
   };

   static_assert(std::is_default_constructible_v<tracy_context> == false,
                 "tracy_context should not be default constructible");
   const auto enable_tracy_profiler = []()
   {
      return [=](auto fun)
      {
         return [=](request &req, auto &&...ctx) -> await_response
         {
            return [](auto const &fun, request &req, auto &&...ctx) -> await_response
            {
               auto verb = to_string(req.get_processing_endpoint().verb);

               auto fiber_id = std::format("{} {} [{}]", string_view{verb.data(), verb.size()},
                                           req.get_raw_path_include_query(), req.get_request_id());

               TracyCZoneCtx zone;
               auto tracy_ctx = tracy_context(fiber_id);
               {
                  auto t_scope = tracy_ctx.enter_scope();
               }
               auto r = (co_await fun(req, tracy_ctx, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...));

               tracy_ctx.status = r.get_status();

               co_return std::move(r);
               TracyCZoneEnd(zone);
               // capture coroutine arguments before suspension
            }(fun, req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
         };
      };
   };
} // namespace spider2