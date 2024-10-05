// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
#define QUXLANG_RES_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
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
} // namespace quxlang

#endif // QUXLANG_TYPE_SIZE_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
