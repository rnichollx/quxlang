// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD
#define QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <cstdint>
#include <rpnx/macros.hpp>

// clang-format off
RPNX_ENUM(quxlang, functanoid_compilation_type, std::uint8_t,
    all,
    runtime,
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
        functanoid_compilation_type compilation_type = functanoid_compilation_type::all;

        RPNX_MEMBER_METADATA(functanoid_requirement_input, functanoid, compilation_type);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_FUNCTANOID_REQUIREMENTS_HEADER_GUARD
