//
// Created by jhrub on 17.12.2022.
//

#pragma once

#include <variant>
#include "overload.h"

namespace spider2 {

    template<class T, class VariantT>
    [[nodiscard]]
    auto try_variant_cast(VariantT &variant) noexcept -> T * {
        return std::visit(overload{
                [&](T &expected) -> T * {
                    return &expected;
                },
                [](auto &anything) -> T * {
                    return nullptr;
                }
        }, variant);
    }

    template<class T, class VariantT>
    [[nodiscard]]
    auto try_variant_cast(VariantT const &variant) noexcept -> const T * {
        return std::visit(overload{
                [&](T const &expected) -> const T * {
                    return &expected;
                },
                [](auto const &anything) -> const T * {
                    return nullptr;
                }
        }, variant);
    }

}