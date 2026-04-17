// Copyright 2023-2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_TEMPLATE_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_TEMPLATE_DECLARATION_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_class.hpp>
#include <quxlang/parsers/function.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/try_parse_variable_declaration.hpp>


namespace quxlang::parsers
{
    declaroid parse_declaroid(parsing_context& ctx);

    inline std::optional< quxlang::ast2_template_declaration > try_parse_template(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        auto pos2 = pos;

        if (!skip_keyword_if_is(pos2, end, "TEMPLATE"))
        {
            return std::nullopt;
        }
        pos = pos2;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' after TEMPLATE");
        }
        std::optional< quxlang::ast2_template_declaration > ct = ast2_template_declaration{};
    get_arg:
        skip_whitespace_and_comments(pos, end);
        declared_parameter arg;

        if (skip_symbol_if_is(pos, end, "@"))
        {
            arg.api_name = parse_argument_name(pos, end);
            if (arg.api_name->empty())
            {
                throw std::logic_error("Expected identifier after '@' in template parameter");
            }

            if (skip_symbol_if_is(pos, end, ":"))
            {
                arg.name = parse_identifier(pos, end);
            }

            if (!skip_whitespace(pos, end))
            {
                throw std::logic_error("Expected whitespace after named template parameter");
            }

            skip_whitespace_and_comments(pos, end);
            arg.type = parse_type_symbol(ctx);

            if (ct->m_template_args.named.contains(*arg.api_name))
            {
                throw std::logic_error("Duplicate named template parameter '" + *arg.api_name + "'");
            }

            ct->m_template_args.named[*arg.api_name] = std::move(arg);
        }
        else
        {
            arg.type = parse_type_symbol(ctx);
            ct->m_template_args.positional.push_back(std::move(arg));
        }
        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }
        else if (skip_symbol_if_is(pos, end, ")"))
        {
            skip_whitespace_and_comments(pos, end);
            ct->m_declaroid = parse_declaroid(ctx);
            ct->location = ctx.get_location_optional(begin, pos);
            return ct;
        }
        else
        {
            throw std::logic_error("Expected ',' or ')' after TEMPLATE(...");
        }
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_TEMPLATE_DECLARATION_HPP
