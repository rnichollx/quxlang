// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_CONSTEXPR_HEADER_GUARD
#define QUXLANG_RES_CONSTEXPR_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/vmir2/vmir2.hpp"

#include <quxlang/res/resolver.hpp>

#include <quxlang/data/constexpr.hpp>

namespace quxlang
{
    struct constexpr_input
    {
        expression expr;
        type_symbol context;
        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;

        RPNX_MEMBER_METADATA(constexpr_input, expr, context, scoped_definitions);
    };

    struct constexpr_input2
    {
        expression expr;
        type_symbol context;
        type_symbol type;

        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;

        RPNX_MEMBER_METADATA(constexpr_input2, expr, context, type, scoped_definitions);
    };



    QUX_CO_RESOLVER(constexpr_bool, constexpr_input, bool);
    QUX_CO_RESOLVER(constexpr_u64, constexpr_input, std::uint64_t);
    QUX_CO_RESOLVER(constexpr_eval, constexpr_input2, constexpr_result);
    QUX_CO_RESOLVER(constexpr_routine, constexpr_input2, vmir2::functanoid_routine3);

}

#endif //CONSTEXPR_HPP
