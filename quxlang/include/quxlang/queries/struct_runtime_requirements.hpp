// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_RUNTIME_REQUIREMENTS_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_RUNTIME_REQUIREMENTS_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Returns layout-independent runtime-header, RTTI, and destructor requirements. */
    struct struct_runtime_requirements_query
    {
        static constexpr auto query_id = "struct_runtime_requirements";
        using input_type = type_symbol;
        using output_type = struct_runtime_requirements;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_RUNTIME_REQUIREMENTS_HEADER_GUARD
