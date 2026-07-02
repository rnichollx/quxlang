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
        out->definition.body = parse_function_block(ctx);
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
