// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_LOCAL_BORROW_ANALYSIS_HEADER_GUARD
#define QUXLANG_VMIR2_LOCAL_BORROW_ANALYSIS_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace quxlang::vmir2
{
    /** Identifies call parameters whose borrowed addresses remain within the call. */
    struct invocation_borrow_summary
    {
        std::set< std::size_t > nonescaping_positional;
        std::set< std::string > nonescaping_named;
    };

    /** Records conservative address escapes through the represented local alias graph. */
    struct local_borrow_analysis
    {
        std::vector< bool > escaped_storage;
        std::vector< bool > escaped_values;
        std::size_t operands = 0;
        std::size_t alias_edges = 0;
        std::size_t traversal_visits = 0;
        std::uint64_t work = 0;
        bool complete = true;
    };

    /**
     * Propagates unknown address uses through a linear represented alias graph.
     * Available call summaries preserve intrinsic borrow lifetimes. Unknown calls
     * and pointer publication widen every reachable addressed object. A supplied
     * budget charges initialization, operand visits, edges, and propagation; an
     * incomplete result provides no escape proof.
     */
    local_borrow_analysis analyze_local_borrows(functanoid_routine3 const& routine,
        std::map< type_symbol, invocation_borrow_summary > const& calls = {}, std::uint64_t* remaining_work = nullptr,
        std::uint64_t maximum_alias_edges = std::numeric_limits< std::uint64_t >::max());
}

#endif // QUXLANG_VMIR2_LOCAL_BORROW_ANALYSIS_HEADER_GUARD
