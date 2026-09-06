// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_MANIPULATORS_DEFER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_DEFER_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /** Resolves defer exits to internal returns without crossing callable boundaries. */
    auto normalize_defer_body(function_block body, std::optional< std::string > label_name) -> function_block;

    /** Produces the immediate callable expression shared by capture analysis and lowering. */
    auto defer_callable_expression(function_defer_statement const& statement) -> expression;
}

#endif
