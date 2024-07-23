//
// Created by jhrub on 04.01.2023.
//

#pragma once

#include <tuple>

namespace spider2 {

    ///This function extracts type T from parameter pack Args args.
    template<class T, class... Args>
    T &get_arg(Args &&... args) {
        return std::get<T &&>(std::forward_as_tuple(std::forward<std::remove_reference_t<Args>>(args)...));
    }

}