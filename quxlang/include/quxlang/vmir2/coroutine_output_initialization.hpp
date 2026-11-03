// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_OUTPUT_INITIALIZATION_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_OUTPUT_INITIALIZATION_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>

#include <cstddef>
#include <map>
#include <optional>
#include <vector>

namespace quxlang::vmir2
{
    /**
     * Verifies captured NVALUE outputs over shared, validated ownership forests.
     * Each root requires one completed value. A partial struct replaces its own
     * obligation with its selected child obligations. A complete value discharges
     * one obligation. The lifetime engine retires children when ownership closes,
     * so every forest has a nonnegative deficit and a zero total proves completion.
     */
    class coroutine_output_initialization
    {
      public:
        /** Registers output roots and statically selected struct ownership edges. */
        coroutine_output_initialization(functanoid_routine3 const& routine, std::map< local_index, type_symbol > const& borrowed_parameters);

        /** Checks one completion snapshot with one visit per previously unseen radix node. */
        bool completed(state_map const& state);

        /** Reports the number of immutable state nodes inspected by completion checks. */
        std::size_t snapshot_visits() const { return summaries_.size(); }

        /** Reports represented ownership edges examined during summary construction. */
        std::size_t ownership_visits() const { return ownership_visits_; }

      private:
        /** Identifies the one selected parent and subobject position of a construction delegate. */
        struct owner
        {
            local_index parent;
            struct_init_selector selector;
        };

        /** Counts outstanding and discharged obligations in one shared radix subtree. */
        struct obligations
        {
            std::size_t required = 0;
            std::size_t completed = 0;
            std::size_t present_roots = 0;
            bool unknown = false;
        };

        /** Retains the immutable source while its aggregate is cached. */
        struct cached_obligations
        {
            state_map::snapshot source;
            obligations counts;
        };

        std::vector< bool > roots_;
        std::vector< bool > members_;
        std::vector< std::optional< owner > > owners_;
        std::size_t root_count_ = 0;
        std::size_t ownership_visits_ = 0;
        bool valid_ = true;
        std::map< state_map::snapshot_node const*, cached_obligations > summaries_;

        /** Joins one immutable node's obligations after its children have been summarized. */
        obligations summarize(state_map::snapshot root);
    };
}

#endif // QUXLANG_VMIR2_COROUTINE_OUTPUT_INITIALIZATION_HEADER_GUARD
