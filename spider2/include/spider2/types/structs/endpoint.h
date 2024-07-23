//
// Created by jhrub on 29.11.2022.
//

#pragma once

#include "spider2/types/platform.h"
#include <boost/algorithm/string.hpp>

namespace spider2 {
    struct endpoint_base {
        http::verb verb = http::verb::get;
        string_view path = string_view{"/"};
    };

    struct match_exact {
        bool operator()(endpoint_base const &app, endpoint_base const &req) const {
            return app.verb == req.verb && app.path == req.path;
        }
    };

    struct match_starts_with {
        bool operator()(endpoint_base const &app, endpoint_base const &req) const {
            return app.verb == req.verb && boost::starts_with(req.path, app.path);
        }
    };

    using matcher_type = std::variant<match_exact, match_starts_with>;

    struct endpoint : public endpoint_base {
        matcher_type matcher = match_exact{};

        inline bool matches(endpoint_base const &req) const {
            return std::visit([&, this](auto &matcher_fun) {
                return matcher_fun(*this, req);
            }, matcher);
        }
    };

}