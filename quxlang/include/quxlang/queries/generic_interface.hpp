// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_GENERIC_INTERFACE_HEADER_GUARD
#define QUXLANG_QUERIES_GENERIC_INTERFACE_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Synthesizes the erased dispatch interface and lifecycle slots of a generic contract. */
    struct generic_interface_query
    {
        static constexpr auto query_id = "generic_interface";
        using input_type = type_symbol;
        using output_type = ast2_interface_declaration;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_GENERIC_INTERFACE_HEADER_GUARD
