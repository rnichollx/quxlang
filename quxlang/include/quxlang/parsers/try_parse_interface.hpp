// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_INTERFACE_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_INTERFACE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <iterator>
#include <optional>
#include <vector>

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/parse_privacy_scope.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/try_parse_function_delegates.hpp>
#include <quxlang/parsers/try_parse_function_return_type.hpp>
#include <quxlang/parsers/try_parse_name.hpp>

namespace quxlang::parsers
{
    std::vector< subdeclaroid > parse_subdeclaroids(parsing_context& ctx);

    /** Tries to parse one interface function with the supplied privacy annotation. */
    inline std::optional< ast2_interface_function_declaration > try_parse_interface_function_declaration(parsing_context& ctx, std::optional< privacy_scope > privacy)
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
        out.privacy = std::move(privacy);
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

    /** Tries to parse one public interface function declaration. */
    inline std::optional< ast2_interface_function_declaration > try_parse_interface_function_declaration(parsing_context& ctx)
    {
        return try_parse_interface_function_declaration(ctx, std::nullopt);
    }

    /** Parses interface functions and flattens scoped PRIVATE blocks. */
    inline auto parse_interface_function_declarations(parsing_context& ctx, std::optional< privacy_scope > inherited_privacy = std::nullopt) -> std::vector< ast2_interface_function_declaration >
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        std::vector< ast2_interface_function_declaration > output;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            parse_iterator closing_probe = pos;
            if (skip_symbol_if_is(closing_probe, end, "}"))
            {
                return output;
            }

            std::optional< privacy_scope > declaration_privacy = try_parse_privacy_scope(ctx);
            if (declaration_privacy.has_value())
            {
                if (inherited_privacy.has_value())
                {
                    throw syntax_compilation_error("PRIVATE interface declarations cannot be nested directly inside a PRIVATE block");
                }

                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "{"))
                {
                    std::vector< ast2_interface_function_declaration > nested = parse_interface_function_declarations(ctx, declaration_privacy);
                    skip_whitespace_and_comments(pos, end);
                    if (!skip_symbol_if_is(pos, end, "}"))
                    {
                        throw syntax_compilation_error("Expected '}' after PRIVATE interface declaration block");
                    }
                    output.insert(output.end(), std::make_move_iterator(nested.begin()), std::make_move_iterator(nested.end()));
                    continue;
                }
            }

            std::optional< ast2_interface_function_declaration > function = try_parse_interface_function_declaration(ctx, declaration_privacy.has_value() ? std::move(declaration_privacy) : inherited_privacy);
            if (!function.has_value())
            {
                throw syntax_compilation_error("Expected interface function declaration");
            }
            output.push_back(std::move(*function));
        }
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

        out.functions = parse_interface_function_declarations(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected '}' after INTERFACE declaration");
        }
        out.location = ctx.get_location_optional(begin, pos);
        return out;
    }

    /** Tries to parse an owning GENERIC or non-owning GENERIC_REF declaration. */
    inline std::optional< ast2_generic_declaration > try_parse_generic(parsing_context& ctx)
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        parse_iterator const begin = pos;

        ast2_generic_declaration out;
        if (skip_keyword_if_is(pos, end, "GENERIC_REF"))
        {
            out.is_reference = true;
        }
        else if (!skip_keyword_if_is(pos, end, "GENERIC"))
        {
            return std::nullopt;
        }

        bool saw_const = false;
        bool saw_incomparable = false;
        bool saw_move_only = false;
        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "CONST"))
            {
                if (!out.is_reference)
                {
                    throw syntax_compilation_error("CONST is only valid on GENERIC_REF declarations");
                }
                if (saw_const)
                {
                    throw syntax_compilation_error("Duplicate CONST modifier on GENERIC_REF declaration");
                }
                saw_const = true;
                out.is_const = true;
                continue;
            }
            if (skip_keyword_if_is(pos, end, "INCOMPARABLE"))
            {
                if (saw_incomparable)
                {
                    throw syntax_compilation_error("Duplicate INCOMPARABLE modifier on generic declaration");
                }
                saw_incomparable = true;
                out.comparable = false;
                continue;
            }
            if (skip_keyword_if_is(pos, end, "MOVE_ONLY"))
            {
                if (saw_move_only)
                {
                    throw syntax_compilation_error("Duplicate MOVE_ONLY modifier on generic declaration");
                }
                saw_move_only = true;
                out.copyable = false;
                continue;
            }
            break;
        }

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw syntax_compilation_error("Expected '{' after generic declaration");
        }

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "IMPLEMENTS"))
            {
                break;
            }
            skip_whitespace_and_comments(pos, end);
            out.implemented_generics.push_back(parse_type_symbol(ctx));
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("Expected ';' after generic IMPLEMENTS declaration");
            }
        }

        out.functions = parse_interface_function_declarations(ctx);
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, "}"))
        {
            throw syntax_compilation_error("Expected '}' after generic declaration");
        }
        out.location = ctx.get_location_optional(begin, pos);
        return out;
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
