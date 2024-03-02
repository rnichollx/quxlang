//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
#define QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/qualified_symbol_reference.hpp"
#include "quxlang/data/symbol_id.hpp"
#include "quxlang/data/type_placement_info.hpp"

namespace quxlang
{
    template < typename Graph >
    auto type_placement_info_from_canonical_type_question_f(Graph* g, type_symbol type) -> rpnx::resolver_coroutine< Graph, type_placement_info >;

    extern template auto type_placement_info_from_canonical_type_question_f< compiler >(compiler* g, type_symbol type) -> rpnx::resolver_coroutine< compiler, type_placement_info >;

    struct type_placement_info_from_canonical_type_question
    {
        template < typename Graph, typename... Ts >
        static auto ask(Graph * g, Ts&&... ts) -> rpnx::resolver_coroutine< Graph, type_placement_info >
        {
            return type_placement_info_from_canonical_type_question_f< Graph >(g, std::forward< Ts >(ts)...);
        }
    };
} // namespace quxlang

#endif // QUXLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
