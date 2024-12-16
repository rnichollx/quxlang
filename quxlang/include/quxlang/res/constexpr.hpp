//
// Created by Ryan Nicholl on 12/14/2024.
//

#ifndef CONSTEXPR_HPP
#define CONSTEXPR_HPP

#include "quxlang/data/expression.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    struct constexpr_input
    {
        expression expr;
        type_symbol context;

        RPNX_MEMBER_METADATA(constexpr_input, expr, context);
    };


    QUX_CO_RESOLVER(constexpr_bool, constexpr_input, bool);
}

#endif //CONSTEXPR_HPP
