//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef RYLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
#define RYLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD

#include "rylang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    template < typename Graph >
    auto type_size_from_canonical_type_question_f(Graph* g, type_symbol type) -> rpnx::resolver_coroutine< Graph, std::size_t >;

    extern template auto type_size_from_canonical_type_question_f< compiler >(compiler* g, type_symbol type) -> rpnx::resolver_coroutine< compiler, std::size_t >;

    struct type_size_from_canonical_type_question
    {
        template < typename Graph, typename... Ts >
        auto ask(Graph * g, Ts&&... ts) -> rpnx::resolver_coroutine< Graph, std::size_t >
        {
            return type_size_from_canonical_type_question_f< Graph >(g, std::forward< Ts >(ts)...);
        }
    };
} // namespace rylang

#endif // RYLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
