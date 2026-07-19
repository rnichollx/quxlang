// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_INTERFACE_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_INTERFACE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/try_parse_function_delegates.hpp>
#include <quxlang/parsers/try_parse_function_return_type.hpp>
#include <quxlang/parsers/try_parse_name.hpp>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);

    inline std::optional< ast2_interface_function_declaration > try_parse_interface_function_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        auto name_opt = try_parse_name(pos, end);
        if (!name_opt.has_value())
        {
            return std::nullopt;
        }

        auto [is_member, name] = std::move(*name_opt);
        if (!is_member)
        {
            throw syntax_compilation_error("Interface functions must be declared with member syntax");
        }
        if (name == "OPERATOR!=" || name == "OPERATOR<" || name == "OPERATOR>" || name == "OPERATOR<=" || name == "OPERATOR>=")
        {
            throw syntax_compilation_error("Comparison declarations must use OPERATOR== or OPERATOR<=>");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            throw syntax_compilation_error("Expected FUNCTION in interface declaration");
        }

        ast2_interface_function_declaration out;
        out.name = std::move(name);
        out.header.call_parameters = parse_function_args(ctx);
        out.definition.return_type = try_parse_function_return_type(ctx);
        out.definition.delegates = parse_function_delegates(ctx);

        skip_whitespace_and_comments(pos, end);
        if (auto body = try_parse_function_block(ctx); body.has_value())
        {
            out.definition.body = std::move(*body);
            out.has_default_body = true;
            out.location = ctx.get_location_optional(begin, pos);
            return out;
        }

        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected interface function body or ';'");
        }

        out.location = ctx.get_location_optional(begin, pos);
        return out;
    }

    inline std::optional< ast2_interface_declaration > try_parse_interface(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "INTERFACE"))
        {
            return std::nullopt;
        }

        ast2_interface_declaration out;
        skip_whitespace_and_comments(pos, end);
        if (skip_keyword_if_is(pos, end, "DEFAULTABLE"))
        {
            out.defaultable = true;
            skip_whitespace_and_comments(pos, end);
        }

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{' after INTERFACE");
        }

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                out.location = ctx.get_location_optional(begin, pos);
                return out;
            }

            auto function = try_parse_interface_function_declaration(ctx);
            if (!function.has_value())
            {
                throw syntax_compilation_error("Expected interface function declaration");
            }
            out.functions.push_back(std::move(*function));
        }
    }

    inline std::optional< ast2_implementation_declaration > try_parse_implementation(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        if (!skip_keyword_if_is(pos, end, "IMPLEMENTATION"))
        {
            return std::nullopt;
        }

        ast2_implementation_declaration out;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' after IMPLEMENTATION");
        }

        skip_whitespace_and_comments(pos, end);
        out.interface_type = parse_type_symbol(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw syntax_compilation_error("Expected ')' after implementation interface type");
        }

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{' after implementation interface type");
        }

        out.declarations = parse_subdeclaroids(ctx);

        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected '}' after implementation declaration");
        }

        out.location = ctx.get_location_optional(begin, pos);
        return out;
    }
} // namespace quxlang::parsers

#endif // QUXLANG_PARSERS_TRY_PARSE_INTERFACE_HEADER_GUARD
