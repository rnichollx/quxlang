// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_EXPRESSION_HEADER_GUARD

#include "quxlang/data/parser_data.hpp"

#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

namespace quxlang::parsers
{
    inline expression parse_expression(parsing_context& ctx);

    inline expression parse_expression(parsing_context& ctx)
    {
        return detail::parse_expression_impl(ctx);
    }
} // namespace quxlang::parsers

#endif // PARSE_EXPRESSION_HPP
