// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD
#define QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD

#include <quxlang/data/antestatal.hpp>
#include <quxlang/data/constexpr.hpp>
#include <quxlang/data/expression.hpp>
#include <quxlang/data/type_symbol.hpp>

#include <map>
#include <optional>
#include <string>

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
        std::optional< type_symbol > antestatal_global_symbol;
        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;

        RPNX_MEMBER_METADATA(constexpr_input2, expr, context, type, antestatal_global_symbol, scoped_definitions);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD
