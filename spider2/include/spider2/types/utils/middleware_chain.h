//
// Created by jhrub on 10.01.2023.
//

#pragma once

#include <memory>
#include "spider2/types.h"

namespace spider2 {


    template<class... Funcs>
    class middleware_chain_builder {
    private:
        using tuple_type = std::tuple<Funcs...>;
        constexpr static size_t s_function_count = std::tuple_size_v<tuple_type>;
        tuple_type functions_;
    public:
        middleware_chain_builder() {}

        middleware_chain_builder(std::tuple<Funcs...> &&functions) :
                functions_(std::move(functions)) {}

        auto operator|(auto &&fun) {
            using fun_type = std::remove_reference_t<decltype(fun)>;
            using fun_type_for_tuple = std::decay_t<fun_type>;

            if constexpr (middleware_chain_builder<Funcs...>::s_function_count == 0) {
                return middleware_chain_builder<fun_type_for_tuple>{
                        std::tuple<fun_type_for_tuple>{std::forward<fun_type>(fun)}
                };
            } else {
                return middleware_chain_builder<Funcs..., fun_type_for_tuple>{
                        std::tuple_cat(std::move(functions_),
                                       std::tuple<fun_type_for_tuple>{std::forward<fun_type>(fun)})
                };
            }
        }

        template<class... Args>
        auto operator()(Args && ...arg) {
            return make_builder(std::make_tuple(std::forward<Args>(arg)...)).build();
        }

    private:

        template<class... Ts>
        auto make_builder(std::tuple<Ts...> args)
        {
            return builder<Ts...>{*this, args};
        }
        template <class... Args>
        struct builder
        {
            middleware_chain_builder<Funcs...>& self;
            std::tuple<Args...> args;
            builder(middleware_chain_builder<Funcs...>& self_, std::tuple<Args...> args_): self(self_), args(args_)
            {}

            auto build()
            {
                return std::apply([this](auto & ...funcs){
                    return build(funcs...);
                }, self.functions_);
            }

            auto build(auto & fun) {
                return std::apply(fun, args);
            }

            template<class Fun, class ... OtherFun>
            auto build(Fun &fun, OtherFun &... funcs ) {
                return fun(build(funcs...));
            }
        };



    };


    inline
    auto begin_middleware() -> middleware_chain_builder<>
    {
        return middleware_chain_builder<>{};
    }

}