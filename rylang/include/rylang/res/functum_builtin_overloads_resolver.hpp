//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP
#define FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    template < typename Graph >
    auto list_builtin_functum_overloads_question_f(Graph* g, qualified_symbol_reference functum) -> rpnx::resolver_coroutine< Graph, std::optional< std::set< call_parameter_information > > >;

    extern template auto list_builtin_functum_overloads_question_f< compiler >(compiler* g, qualified_symbol_reference functum) -> rpnx::resolver_coroutine< compiler, std::optional< std::set< call_parameter_information > > >;

    struct list_builtin_functum_overloads_question
    {
        template < typename Graph, typename... Ts >
        auto ask(Ts&&... ts) -> rpnx::resolver_coroutine< Graph, std::optional< std::set< call_parameter_information > > >
        {
            return list_builtin_functum_overloads_question_f< Graph >(std::forward< Ts >(ts)...);
        }
    };

} // namespace rylang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HPP
