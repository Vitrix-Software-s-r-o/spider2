//
// Created by jhrub on 17.12.2022.
//

#pragma once

#include <memory>

namespace spider2 {

    /**
     * This is to easy create an overload set from lambdas
     *
     * overload{[](int x){}, [](float x){ })(2.f);
     *
     * @tparam FunT - lambda
     */
    template<class... FunT>
    struct overload : public FunT ... {
        overload(FunT &&... fnc) : FunT(std::forward<FunT>(fnc))... {}

        using FunT::operator() ...;
    };
}