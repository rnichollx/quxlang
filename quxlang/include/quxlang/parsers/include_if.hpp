//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_INCLUDE_IF_HEADER
#define RPNX_QUXLANG_INCLUDE_IF_HEADER

#include <optional>
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/skip_whitespace.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/skip_symbol_if_is.hpp>


namespace quxlang::parsers
{
    template < typename It >
    std::optional< expression > try_parse_include_if(It& pos, It end)
    {
        if (!skip_keyword_if_is(pos, end, "INCLUDE_IF"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::runtime_error("expected ( after INCLUDE_IF");
        }

        skip_whitespace_and_comments(pos, end);

        expression out;

        out = parse_expression(pos, end);

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::runtime_error("expected ) after INCLUDE_IF condition");
        }

        skip_whitespace_and_comments(pos, end);

        return out;
    }

} // namespace quxlang::parsers

#endif // RPNX_QUXLANG_INCLUDE_IF_HEADER
