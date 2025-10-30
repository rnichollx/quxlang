// Copyright (c) 2025 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_DATA_STATEMENTS_HEADER_GUARD
#define QUXLANG_DATA_STATEMENTS_HEADER_GUARD
#include "expression.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "rpnx/variant.hpp"

namespace quxlang
{

    struct function_place_statement
    {
        // The expression which yields a pointer to the location to place the object.
        expression at;

        // Type to place.
        type_symbol type;

        // Optional assignment initializer,
        // If present, args must be empty.
        // e.g. PLACE AT(loc) type := assign_init_expr;
        std::optional<expression> assign_init;

        // Optional constructor args,
        // e.g. PLACE AT(loc) type :(args...);
        std::vector<expression_arg> args;

        RPNX_MEMBER_METADATA(function_place_statement, at, type, assign_init, args);
    };

    struct function_destroy_statement
    {
        // The expression which yields a pointer to the location to destroy the object.
        expression at;
        // Type to destroy
        type_symbol type;

        RPNX_MEMBER_METADATA(function_destroy_statement, at, type);
    };
}

#endif // QUXLANG_STATEMENTS_HPP
