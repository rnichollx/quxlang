// Copyright 2023-2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_VARIABLE_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_VARIABLE_DECLARATION_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
{
    inline std::optional< ast2_variable_declaration > try_parse_variable_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        skip_whitespace_and_comments(pos, end);
        std::optional< ast2_variable_declaration > output;

        std::set< std::string > keyword_tags;
        if (!skip_keyword_if_is(pos, end, "VAR"))
        {
            if (skip_keyword_if_is(pos, end, "STATIC"))
            {
                keyword_tags.insert("STATIC");
            }
            else
            {
                return output;
            }
        }

        skip_whitespace_and_comments(pos, end);

        while (true)
        {
            if (skip_keyword_if_is(pos, end, "CONSTEXPR_READABLE"))
            {
                keyword_tags.insert("CONSTEXPR_READABLE");
            }
            else if (skip_keyword_if_is(pos, end, "CONSTEXPR_READWRITE"))
            {
                keyword_tags.insert("CONSTEXPR_READWRITE");
            }
            else
            {
                break;
            }

            skip_whitespace_and_comments(pos, end);
        }

        type_symbol type = try_parse_type_symbol(ctx).value();

        skip_whitespace_and_comments(pos, end);

        std::optional< expression > init_expr;
        std::vector< expression_arg > init_args;

        if (skip_symbol_if_is(pos, end, ":("))
        {
            while (true)
            {
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }

                init_args.push_back(parse_expression_arg(ctx));

                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ","))
                {
                    continue;
                }

                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw std::logic_error("Expected ',' or ')' after VAR initializer arguments");
                }

                break;
            }

            skip_whitespace_and_comments(pos, end);
        }
        else if (skip_symbol_if_is(pos, end, ":="))
        {
            skip_whitespace_and_comments(pos, end);
            init_expr = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
        }

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ';' after VAR declaration");
        }

        output = ast2_variable_declaration{};

        output->type = type;
        output->keyword_tags = std::move(keyword_tags);
        output->init_expr = std::move(init_expr);
        output->init_args = std::move(init_args);
        output->location = ctx.get_location_optional(begin, pos);

        return output;
    }

} // namespace quxlang

#endif //TRY_PARSE_VARIABLE_DECLARATION_HPP
