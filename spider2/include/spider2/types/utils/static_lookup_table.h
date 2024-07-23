//
// Created by jhrub on 16.02.2023.
//

#pragma once

#include "../platform.h"

namespace spider2 {
    template<class K, class V, size_t N>
    class static_lookup_table {
        constexpr static bool use_binary_search = N > 50;
        std::array<K, N> keys_;
        std::array<V, N> values_;
    public:
        constexpr static_lookup_table(std::array<std::pair<K, V>, N> data) {
            if constexpr (use_binary_search) {
                std::sort(data.begin(), data.end(), [](std::pair<K, V> const &a, std::pair<K, V> const &b) {
                    return a.first < b.first;
                });
            }

            for (size_t i = 0; i != N; ++i) {
                auto &[key, value] = data[i];
                keys_[i] = key;
                values_[i] = value;
            }
        }

        constexpr optional<V> lookup(K const &key) const {
            if constexpr (use_binary_search) {
                auto it = std::lower_bound(keys_.begin(), keys_.end(), key);
                if (it != keys_.end() && *it == key) {
                    return {values_[std::distance(keys_.begin(), it)]};
                }
                return {};
            } else {
                auto it = std::find(keys_.begin(), keys_.end(), key);
                if (it != keys_.end()) {
                    return {values_[std::distance(keys_.begin(), it)]};
                }
                return {};
            }
        }

    };
}