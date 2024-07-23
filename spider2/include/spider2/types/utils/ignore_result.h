//
// Created by jhrub on 28.02.2023.
//

#pragma once

namespace spider2 {
    /// it serves as visible ignoring for [[nodiscard]]
    auto IGNORE_RESULT(auto &&) noexcept -> void {
    }
}