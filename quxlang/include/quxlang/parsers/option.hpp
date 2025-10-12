// Copyright (c) 2025 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_OPTION_HPP
#define QUXLANG_OPTION_HPP

#include <optional>
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/parse_expression.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_option > try_parse_option(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "OPTION"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos, end);

        ast2_option opt;

        if (skip_keyword_if_is(pos, end, "NUMBER"))
        {
            opt.kind = option_kind::number;
        }
        else if (skip_keyword_if_is(pos, end, "STRING"))
        {
            opt.kind = option_kind::string;
        }
        else if (skip_keyword_if_is(pos, end, "BOOL"))
        {
            opt.kind = option_kind::boolean;
        }
        else
        {
            throw std::logic_error("Expected NUMBER, STRING, or BOOL after OPTION");
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_keyword_if_is(pos, end, "DEFAULT"))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected ( after DEFAULT");
            }
            skip_whitespace_and_comments(pos, end);
            expression e = parse_expression(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::logic_error("Expected ) after DEFAULT expression");
            }
            opt.default_value = e;
            skip_whitespace_and_comments(pos, end);
        }

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';' after OPTION declaration");
        }

        return opt;
    }
}

#endif // QUXLANG_OPTION_HPP
