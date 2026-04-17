// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_VAR_STATEMENT_HEADER_GUARD
#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>

namespace quxlang::parsers
{
    inline function_var_statement parse_var_statement(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "VAR"))
        {
            throw std::logic_error("Expected 'VAR'");
        }

        skip_whitespace_and_comments(pos, end);

        function_var_statement var_statement;

        var_statement.name = parse_identifier(pos, end);

        skip_whitespace_and_comments(pos, end);

        var_statement.type = parse_type_symbol(ctx);

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ":("))
        {
            while (true)
            {
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }

                expression expr = parse_expression(ctx);
                var_statement.initializers.push_back(std::move(expr));

                if (skip_symbol_if_is(pos, end, ","))
                {
                    continue;
                }
                else if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }
                else
                {
                    throw std::logic_error("Expected ',' or ')'");
                }
            }
        }
        if (skip_symbol_if_is(pos, end, ":["))
        {
            while (true)
            {
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }

                expression expr = parse_expression(ctx);
                throw rpnx::unimplemented();

                if (skip_symbol_if_is(pos, end, ","))
                {
                    continue;
                }
                else if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                else
                {
                    throw std::logic_error("Expected ',' or ')'");
                }
            }
        }
        else if (skip_symbol_if_is(pos, end, ":="))
        {
            skip_whitespace_and_comments(pos, end);
            var_statement.equals_initializer = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';'");
        }

        var_statement.location = ctx.get_location_optional(begin, pos);
        return var_statement;
    }

} // namespace quxlang

#endif // PARSE_VAR_STATEMENT_HPP
