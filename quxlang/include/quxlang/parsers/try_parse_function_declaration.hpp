// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DECLARATION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <optional>

#include <quxlang/ast2/ast2_function_delegate.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/try_parse_function_delegates.hpp>
#include <quxlang/parsers/try_parse_function_return_type.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

namespace quxlang::parsers
{
    function_block parse_function_block(parsing_context& ctx);

    /** Parses one VIRTUAL suffix and its optional slot constraints. */
    inline auto try_parse_function_virtual_specifier(parsing_context& ctx) -> std::optional< ast2_virtual_specifier >
    {
        parse_iterator& pos = ctx.iter_pos;
        parse_iterator const end = ctx.iter_end;
        skip_whitespace_and_comments(pos, end);
        parse_iterator const begin = pos;
        if (!skip_keyword_if_is(pos, end, "VIRTUAL"))
        {
            return std::nullopt;
        }

        ast2_virtual_specifier result;
        skip_whitespace_and_comments(pos, end);
        if (skip_symbol_if_is(pos, end, "("))
        {
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("VIRTUAL() requires at least one option");
            }

            while (true)
            {
                std::optional< std::string_view > option = skip_keyword_if_one_of(pos, end, {"OVERRIDE", "FINAL", "PURE"});
                if (!option.has_value())
                {
                    throw syntax_compilation_error("Expected OVERRIDE, FINAL, or PURE in VIRTUAL options");
                }
                if (*option == "OVERRIDE")
                {
                    if (result.is_override)
                    {
                        throw syntax_compilation_error("Duplicate OVERRIDE in VIRTUAL options");
                    }
                    result.is_override = true;
                }
                else if (*option == "FINAL")
                {
                    if (result.is_final)
                    {
                        throw syntax_compilation_error("Duplicate FINAL in VIRTUAL options");
                    }
                    result.is_final = true;
                }
                else
                {
                    if (result.is_pure)
                    {
                        throw syntax_compilation_error("Duplicate PURE in VIRTUAL options");
                    }
                    result.is_pure = true;
                }

                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, ")"))
                {
                    break;
                }
                if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ')' in VIRTUAL options");
                }
                skip_whitespace_and_comments(pos, end);
            }
        }
        if (result.is_final && result.is_pure)
        {
            throw syntax_compilation_error("VIRTUAL FINAL and PURE cannot be combined");
        }
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

    inline std::optional< ast2_function_declaration > try_parse_function_declaration(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        std::optional< ast2_function_declaration > out;

        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            return out;
        }
        out = ast2_function_declaration{};

        out->header.call_parameters = parse_function_args(ctx);

        bool has_this_parameter = false;
        while (true)
        {
            if (std::optional< ast2_function_parameter > this_parameter = try_parse_function_this_parameter(ctx); this_parameter.has_value())
            {
                if (has_this_parameter)
                {
                    throw syntax_compilation_error("A function cannot declare more than one THIS qualifier");
                }
                has_this_parameter = true;
                out->header.call_parameters.insert(out->header.call_parameters.begin(), std::move(*this_parameter));
                continue;
            }

            if (std::optional< ast2_virtual_specifier > virtual_specifier = try_parse_function_virtual_specifier(ctx); virtual_specifier.has_value())
            {
                if (out->header.virtual_specifier.has_value())
                {
                    throw syntax_compilation_error("A function cannot declare more than one VIRTUAL specifier");
                }
                if (out->header.is_nonvirtual)
                {
                    throw syntax_compilation_error("VIRTUAL and NONVIRTUAL cannot be combined");
                }
                out->header.virtual_specifier = std::move(*virtual_specifier);
                continue;
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "NONVIRTUAL"))
            {
                if (out->header.is_nonvirtual)
                {
                    throw syntax_compilation_error("A function cannot declare NONVIRTUAL more than once");
                }
                if (out->header.virtual_specifier.has_value())
                {
                    throw syntax_compilation_error("VIRTUAL and NONVIRTUAL cannot be combined");
                }
                out->header.is_nonvirtual = true;
                continue;
            }
            break;
        }

        skip_whitespace_and_comments(pos, end);
        if (skip_keyword_if_is(pos, end, "ENABLE_IF"))
        {
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after ENABLE_IF");
            }
            skip_whitespace_and_comments(pos, end);
            out->header.enable_if = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after ENABLE_IF expression");
            }
            skip_whitespace_and_comments(pos, end);
        }

        out->definition.return_type = try_parse_function_return_type(ctx);
        out->definition.delegates = parse_function_delegates(ctx);
        if (out->header.virtual_specifier.has_value() && out->header.virtual_specifier->is_pure)
        {
            if (!out->definition.delegates.empty())
            {
                throw syntax_compilation_error("A VIRTUAL(PURE) function cannot declare constructor delegates");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw syntax_compilation_error("A VIRTUAL(PURE) function declaration must end with ';'");
            }
        }
        else
        {
            out->definition.body = parse_function_block(ctx);
        }
        out->location = ctx.get_location_optional(begin, pos);
        return out;
    }

    inline std::optional< ast2_test > try_parse_test(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;

        std::optional< ast2_test > out;

        ast2_test_mode mode;
        if (skip_keyword_if_is(pos, end, "STATIC_TEST"))
        {
            mode = ast2_test_mode::static_only;
        }
        else if (skip_keyword_if_is(pos, end, "UNIT_TEST"))
        {
            mode = ast2_test_mode::unit_only;
        }
        else if (skip_keyword_if_is(pos, end, "DUAL_TEST"))
        {
            mode = ast2_test_mode::dual;
        }
        else
        {
            return out;
        }

        out = ast2_test{.mode = mode};

        skip_whitespace_and_comments(pos, end);
        if (skip_keyword_if_is(pos, end, "EXPECT_FAIL"))
        {
            if (mode != ast2_test_mode::static_only)
            {
                throw syntax_compilation_error("Only STATIC_TEST supports expectation modifiers");
            }
            out->expected_mode = static_test_expected_mode::expect_fail;
        }
        else if (skip_keyword_if_is(pos, end, "EXPECT_COMPILATION_FAILURE"))
        {
            if (mode != ast2_test_mode::static_only)
            {
                throw syntax_compilation_error("Only STATIC_TEST supports expectation modifiers");
            }
            out->expected_mode = static_test_expected_mode::expect_compilation_failure;
        }

        out->definition.body = parse_function_block(ctx);
        out->location = ctx.get_location_optional(begin, pos);
        return out;
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_DECLARATION_HPP
