// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_RETURN_STATEMENT_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include <optional>

namespace quxlang
{
    struct function_return_statement
    {
        std::optional<expression> expr;

        RPNX_MEMBER_METADATA(function_return_statement, expr);
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_STATEMENT_HEADER_GUARD
