// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_ASM_ASM_CALLABLE_HEADER_GUARD
#define QUXLANG_PARSERS_ASM_ASM_CALLABLE_HEADER_GUARD
#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/parsers/parse_symbol.hpp"
#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/keyword.hpp"
#include "quxlang/parsers/symbol.hpp"
#include "quxlang/parsers/asm/parse_register.hpp"
#include "quxlang/parsers/parse_identifier.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"

#include <utility>

namespace quxlang::parsers
{

    inline std::optional< ast2_asm_callable > try_parse_asm_callable(parsing_context& ctx)
    {
        ast2_asm_callable output;
        auto trial = ctx;
        auto& pos = trial.iter_pos;
        auto end = trial.iter_end;

        skip_whitespace_and_comments(pos, end);
        if (!parsers::skip_keyword_if_is(pos, end, "CALLABLE"))
        {
            return std::nullopt;
        }
        parsers::skip_whitespace_and_comments(pos, end);

        // TODO: Insert calling convention here?

        if (parsers::skip_keyword_if_is(pos, end, "CALLCONV"))
        {
            parsers::skip_whitespace_and_comments(pos, end);

            std::string callconv = parsers::parse_identifier(pos, end);

            output.calling_conv = std::move(callconv);

            parsers::skip_whitespace_and_comments(pos, end);
        }

        else
        {
            output.calling_conv = "default";
        }

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '(' in CALLABLE expression");
        }

        while (true)
        {
            parsers::skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            auto register_name = parse_register(pos, end);

            parsers::skip_whitespace_and_comments(pos, end);

            auto input_type = parsers::parse_type_symbol(trial);

            output.args.push_back(ast2_argument_interface{.register_name = std::move(register_name), .type = std::move(input_type)});

            parsers::skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("Expected ',' in CALLABLE expression");
            }
        }

        ctx.iter_pos = pos;
        return std::move(output);

    }

}

#endif //ASM_CALLABLE_HPP
