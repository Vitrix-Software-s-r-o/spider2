//
// Created by jhrub on 6/1/2023.
//

#pragma once
#include "../types.h"
namespace spider2
{
    const auto allow_origin_middleware = [](vector<string> allowed_origin_list, bool vary_origin = true)
    {
        return [=](auto fun)
        {
            return [=](request &req, auto &&...ctx) -> await_response
            {
                return [](auto const& fun, request &req, vector<string> const &allowed_origin_list, bool vary_origin,
                          auto &&...ctx) -> await_response
                {
                    auto &headers = req.get_headers();

                    auto allowed_origin = [&]() -> optional<string>
                    {
                        if (auto origin = headers.find(http::field::origin); origin != headers.end())
                        {
                            auto origin_header_value = static_cast<string>(origin->value());
                            if (auto end = allowed_origin_list.end();
                                std::find(allowed_origin_list.begin(), end, origin_header_value) != end)
                            {
                                return origin_header_value;
                            }
                        }

                        return {};
                    }();

                    if (allowed_origin.has_value())
                    {
                        auto resp = co_await fun(req, std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
                        resp.try_set_header(http::field::access_control_allow_origin, allowed_origin.value());
                        if (vary_origin)
                        {
                            resp.try_set_header(http::field::vary, "Origin");
                        }

                        co_return resp;
                    }
                    else
                    {
                        co_return response::return_string(http::status::forbidden, "403 - forbidden", "text/plain");
                    }
                }(fun, req, allowed_origin_list, vary_origin,
                  std::forward<std::remove_reference_t<decltype(ctx)>>(ctx)...);
            };
        };
    };
} // namespace spider2
