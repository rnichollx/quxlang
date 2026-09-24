// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_COMPILATION_POLICY_HEADER_GUARD
#define QUXLANG_DATA_COMPILATION_POLICY_HEADER_GUARD

#include <quxlang/data/build_type.hpp>
#include <map>

/** Selects a lowering-time policy. Boolean policies use ordinal zero for disabled and one for enabled. */
RPNX_ENUM(quxlang, compilation_policy, std::uint8_t, policy_assert_enabled, policy_check_bounds, policy_check_overflow, policy_unimplemented_panics);

namespace quxlang
{
    /** Concrete branch selections for one lowering target. */
    struct compilation_policies
    {
        std::map< compilation_policy, std::size_t > selections;

        /** Returns the selected branch ordinal, defaulting to enabled for constexpr evaluation. */
        auto selection(compilation_policy policy) const -> std::size_t
        {
            return selections.contains(policy) ? selections.at(policy) : 1;
        }

        RPNX_MEMBER_METADATA(compilation_policies, selections);
    };

    /** Resolves boolean policy defaults and explicit per-output overrides. */
    inline auto resolve_compilation_policies(build_type build, std::map< compilation_policy, bool > const& overrides) -> compilation_policies
    {
        compilation_policies result;
        bool enabled = build_type_has_debug_information(build) && build != build_type::release_dbgsym;
        for (compilation_policy policy : {compilation_policy::policy_assert_enabled, compilation_policy::policy_check_bounds, compilation_policy::policy_check_overflow})
        {
            result.selections[policy] = overrides.contains(policy) ? overrides.at(policy) : enabled;
        }
        bool unimplemented_panics = build != build_type::release && build != build_type::release_dbgsym;
        result.selections[compilation_policy::policy_unimplemented_panics] = overrides.contains(compilation_policy::policy_unimplemented_panics) ? overrides.at(compilation_policy::policy_unimplemented_panics) : unimplemented_panics;
        return result;
    }
}
#endif
