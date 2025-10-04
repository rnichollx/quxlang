//
// Created by Ryan Nicholl on 12/14/2024.
//

#ifndef QUXLANG_RES_CONSTEXPR_HEADER_GUARD
#define QUXLANG_RES_CONSTEXPR_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/vmir2/vmir2.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    struct constexpr_input
    {
        expression expr;
        type_symbol context;

        RPNX_MEMBER_METADATA(constexpr_input, expr, context);
    };

    struct constexpr_input2
    {
        expression expr;
        type_symbol context;
        type_symbol type;

        RPNX_MEMBER_METADATA(constexpr_input2, expr, context, type);
    };

    struct constexpr_result
    {
        cow<type_symbol> type;
        cow<std::vector<std::byte>> value;

        RPNX_MEMBER_METADATA(constexpr_result, type, value);
    };

    QUX_CO_RESOLVER(constexpr_bool, constexpr_input, bool);
    QUX_CO_RESOLVER(constexpr_u64, constexpr_input, std::uint64_t);
    QUX_CO_RESOLVER(constexpr_eval, constexpr_input2, constexpr_result);
    QUX_CO_RESOLVER(constexpr_routine, constexpr_input2, vmir2::functanoid_routine3);

}

#endif //CONSTEXPR_HPP
