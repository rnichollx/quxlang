//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_QUESTION_HEADER

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
    auto type_placement_info_from_canonical_type_question_f(Graph* g, qualified_symbol_reference type) -> rpnx::resolver_coroutine< Graph, type_placement_info >;

    extern template auto type_placement_info_from_canonical_type_question_f< compiler >(compiler* g, qualified_symbol_reference type) -> rpnx::resolver_coroutine< compiler, type_placement_info >;

    struct type_placement_info_from_canonical_type_question
    {
        template < typename Graph, typename... Ts >
        static auto ask(Graph * g, Ts&&... ts) -> rpnx::resolver_coroutine< Graph, type_placement_info >
        {
            return type_placement_info_from_canonical_type_question_f< Graph >(g, std::forward< Ts >(ts)...);
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_TYPE_PLACEMENT_INFO_FROM_CANONICAL_TYPE_RESOLVER_HEADER
