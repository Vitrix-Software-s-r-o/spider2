//
// Created by jhrub on 16.02.2023.
//

#pragma once

#include "../platform.h"
#include "../utils.h"

namespace spider2 {

    constexpr auto make_default_mime_table() {
        using namespace std::string_view_literals;
        return static_lookup_table{std::array{std::pair{".exe"sv, "application/binary"sv},
                                              std::pair{".bin"sv, "application/binary"sv},
                                              std::pair{".msi"sv, "application/binary"sv},
                                              std::pair{".js"sv, "text/javascript"sv},
                                              std::pair{".css"sv, "text/css"sv},
                                              std::pair{".txt"sv, "text/plain"sv},
                                              std::pair{".png"sv, "image/png"sv},
                                              std::pair{".jpg"sv, "image/jpeg"sv},
                                              std::pair{".jpeg"sv, "image/jpeg"sv},
                                              std::pair{".jpe"sv, "image/jpeg"sv},
                                              std::pair{".woff"sv, "application/font-woff"sv},
                                              std::pair{".otf"sv, "font/opentype"sv},
                                              std::pair{".eot"sv, "application/vnd.ms-fontobject"sv},
                                              std::pair{".ttf"sv, "font/truetype"sv},
                                              std::pair{".woff"sv, "font/woff2"sv},
                                              std::pair{".svg"sv, "image/svg+xml"sv},
                                              std::pair{".xml"sv, "text/xml"sv},
                                              std::pair{".html"sv, "text/html"sv},
                                              std::pair{".htm"sv, "text/html"sv}}};
    }

    class mime_table {
    public:
        constexpr static auto default_table = make_default_mime_table();

        inline mime_table(optional<string_map> user_table = {}) : user_table_(
                std::move(user_table)) {
        }

        inline
        string_view lookup(string_view extension_with_dot, string_view fallback = "application/binary") const {
            if (user_table_.has_value()) {
                auto &table = user_table_.value();
                auto it = table.find(extension_with_dot);
                if (it != table.end()) {
                    return it->second;
                }
            }

            return default_table.lookup(extension_with_dot).value_or(fallback);
        }

    private:
        optional<string_map> user_table_;
    };

}