// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_EXPR_IR2_HEADER_GUARD
#define QUXLANG_RES_EXPR_IR2_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "rpnx/resolver_utilities.hpp"

#include <quxlang/res/resolver.hpp>

#include <quxlang/data/expression.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include <quxlang/data/type_symbol.hpp>

namespace quxlang
{
    struct expr_ir2_input
    {
        expression expr;
        type_symbol context;

        RPNX_MEMBER_METADATA(expr_ir2_input, expr, context);
    };
    // TODO: Implement this

    QUX_CO_RESOLVER(expr_ir2, expr_ir2_input, vmir2::functanoid_routine2);
} // namespace quxlang

#endif // RPNX_QUXLANG_EXPR_IR2_HEADER
