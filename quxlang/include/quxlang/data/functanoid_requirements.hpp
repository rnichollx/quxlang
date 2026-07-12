// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD
#define QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <cstdint>
#include <rpnx/macros.hpp>

// clang-format off
RPNX_ENUM(quxlang, dependency_set, std::uint8_t,
    native,
    constexpr_
)
// clang-format on

namespace quxlang
{
    /**
     * Input shared by direct functanoid requirement queries.
     */
    struct functanoid_requirement_input
    {
        instanciation_reference functanoid;
        dependency_set dependencies = dependency_set::native;

        RPNX_MEMBER_METADATA(functanoid_requirement_input, functanoid, dependencies);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD
