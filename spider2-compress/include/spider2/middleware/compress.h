//
// Created by jhrub on 5/10/2023.
//

#pragma once
#include <fmt/format.h>
#include <spider2/types.h>

namespace spider2
{
   namespace brotli
   {
      auto compress(string_view buffer, int quality) -> string;
      auto decompress(string_view buffer) -> string;
   } // namespace brotli

   /// Setting the field to false will disable compression
   constexpr std::string_view compress_field = "spider2-compress";

   /// the value false for compress_field
   constexpr std::string_view compress_field_disable_val = "false";

   struct compress_middleware
   {
      int quality = 5;

      auto operator()(auto &&fun)
      {
         const int quality_temp = this->quality;
         return [fun = std::forward<std::remove_reference_t<decltype(fun)>>(fun),
                 quality_temp](request &req, auto &&...ctx) -> await_response
         {
            return [](auto fun, const int quality_local, request &req, auto &&...ctx) -> await_response
            {
               bool can_compress_brotli = [&]()
               {
                  auto &fields = req.get_headers();
                  if (auto it = fields.find(http::field::accept_encoding); it != fields.end())
                  {
                     auto value = it->value();
                     if (value.find("br") != boost::beast::string_view::npos)
                     {
                        return true;
                     }
                  }
                  return false;
               }();

               auto resp = co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);

               if (can_compress_brotli)
               {
                  std::visit(overload{[=](http::response<http::string_body> &msg) -> void
                                      {
                                         auto ce = msg[http::field::content_encoding];
                                         if (!ce.empty() && (ce == "br" || ce == "gzip" || ce == "deflate"))
                                         {
                                            return;
                                         }

                                         if (msg[compress_field] == compress_field_disable_val)
                                         {
                                            msg.erase(compress_field);
                                            return;
                                         }

                                         auto compressed_body = brotli::compress(msg.body(), quality_local);
                                         if (!compressed_body.empty() && compressed_body.size() < msg.body().size())
                                         {
                                            msg.body() = std::move(compressed_body);
                                            msg.content_length(msg.body().size());

                                            if (!ce.empty())
                                            {
                                               msg.set(http::field::content_encoding,
                                                       fmt::format("{},br", std::string_view{ce.data(), ce.size()}));
                                            }
                                            else
                                            {
                                               msg.set(http::field::content_encoding, "br");
                                            }
                                         }
                                      },
                                      [](auto &) -> void {

                                      }},
                             resp.message);
               }

               co_return resp;
            }(fun, quality_temp, req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
         };
      }
   };
} // namespace spider2