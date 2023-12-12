//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD
#define RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/symbol_id.hpp"
#include "rylang/data/type_placement_info.hpp"

namespace rylang
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
} // namespace rylang

#endif // RYLANG_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER_GUARD
