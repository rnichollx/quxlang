// Copyright 2023-2024, 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_TEMPLATE_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_TEMPLATE_DECLARATION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_struct.hpp>
#include <quxlang/parsers/function.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/try_parse_variable_declaration.hpp>


namespace quxlang::parsers
{
    declaroid parse_declaroid(parsing_context& ctx);

    inline auto parse_template_parameter_kind(parsing_context& ctx) -> template_parameter_kind
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        if (skip_keyword_if_is(pos, end, "TYPE"))
        {
            return template_parameter_kind::type;
        }
        if (skip_keyword_if_is(pos, end, "VALUE"))
        {
            return template_parameter_kind::value;
        }

        throw syntax_compilation_error("Expected TYPE or VALUE in template parameter");
    }

    inline auto default_template_type(declared_parameter const& arg) -> type_symbol
    {
        std::string name;
        if (arg.name.has_value())
        {
            name = *arg.name;
        }
        else if (arg.api_name.has_value())
        {
            name = *arg.api_name;
        }
        return type_temploidic{.name = std::move(name)};
    }

    inline auto parse_template_parameter_type(parsing_context& ctx, declared_parameter const& arg) -> type_symbol
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;

        skip_whitespace_and_comments(pos, end);
        auto lookahead = pos;
        if (arg.kind == template_parameter_kind::type && (skip_symbol_if_is(lookahead, end, ",") || skip_symbol_if_is(lookahead, end, ")")))
        {
            return default_template_type(arg);
        }
        return parse_type_symbol(ctx);
    }

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
            throw syntax_compilation_error("Expected '(' after TEMPLATE");
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
                throw syntax_compilation_error("Expected identifier after '@' in template parameter");
            }

            if (skip_symbol_if_is(pos, end, ":"))
            {
                arg.name = parse_identifier(pos, end);
            }

            if (!skip_whitespace(pos, end))
            {
                throw syntax_compilation_error("Expected whitespace after named template parameter");
            }

            skip_whitespace_and_comments(pos, end);
            arg.kind = parse_template_parameter_kind(ctx);
            arg.type = parse_template_parameter_type(ctx, arg);

            if (ct->m_template_args.named.contains(*arg.api_name))
            {
                throw syntax_compilation_error("Duplicate named template parameter '" + *arg.api_name + "'");
            }

            ct->m_template_args.named[*arg.api_name] = std::move(arg);
        }
        else
        {
            arg.kind = parse_template_parameter_kind(ctx);
            arg.type = parse_template_parameter_type(ctx, arg);
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
            throw syntax_compilation_error("Expected ',' or ')' after TEMPLATE(...");
        }
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_TEMPLATE_DECLARATION_HPP
