// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_ASM_ASM_CALLABLE_HEADER_GUARD
#define QUXLANG_PARSERS_ASM_ASM_CALLABLE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/parsers/parse_symbol.hpp"
#include "quxlang/parsers/parse_whitespace_and_comments.hpp"
#include "quxlang/parsers/keyword.hpp"
#include "quxlang/parsers/function.hpp"
#include "quxlang/parsers/symbol.hpp"
#include "quxlang/parsers/asm/parse_register.hpp"
#include "quxlang/parsers/parse_identifier.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"

#include <utility>

namespace quxlang::parsers
{

    /**
     * Parses the CALLABLE surface for ASM_PROCEDURE declarations.
     *
     * ASM_PROCEDURE follows a normal calling convention, so argument and return
     * registers are not part of the source surface.
     */
    inline std::optional< ast2_asm_callable > try_parse_asm_procedure_callable(parsing_context& ctx)
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

        if (parsers::skip_keyword_if_is(pos, end, "CALLCONV"))
        {
            parsers::skip_whitespace_and_comments(pos, end);
            std::string callconv = parsers::parse_keyword(pos, end);
            if (callconv.empty())
            {
                throw syntax_compilation_error("Expected calling convention name in ASM_PROCEDURE CALLABLE expression");
            }
            output.calling_conv = std::move(callconv);
            parsers::skip_whitespace_and_comments(pos, end);
        }
        else
        {
            output.calling_conv = "CCALL";
        }

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' in ASM_PROCEDURE CALLABLE expression");
        }

        bool parsing_return = false;
        while (true)
        {
            parsers::skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            if (parsing_return || skip_symbol_if_is(pos, end, ";"))
            {
                parsing_return = true;
                parsers::skip_whitespace_and_comments(pos, end);
                if (!parsers::skip_keyword_if_is(pos, end, "RETURN"))
                {
                    throw syntax_compilation_error("Expected RETURN after ';' in ASM_PROCEDURE CALLABLE expression");
                }

                parsers::skip_whitespace_and_comments(pos, end);
                output.return_type = parsers::parse_type_symbol(trial);
                parsers::skip_whitespace_and_comments(pos, end);

                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw syntax_compilation_error("Expected ')' after ASM_PROCEDURE CALLABLE return declaration");
                }
                break;
            }

            std::optional< std::string > api_name;
            if (skip_symbol_if_is(pos, end, "@"))
            {
                api_name = parsers::parse_argument_name(pos, end);
                parsers::skip_whitespace_and_comments(pos, end);
            }

            type_symbol input_type = parsers::parse_type_symbol(trial);

            output.args.push_back(ast2_argument_interface{.api_name = std::move(api_name), .register_name = std::nullopt, .type = std::move(input_type)});

            parsers::skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            if (skip_symbol_if_is(pos, end, ";"))
            {
                parsing_return = true;
                continue;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' in ASM_PROCEDURE CALLABLE expression");
            }
        }

        ctx.iter_pos = pos;
        return std::move(output);

    }

    /**
     * Parses the CALLABLE surface for ASM_INLINE_FUNCTION declarations.
     *
     * ASM_INLINE_FUNCTION keeps explicit register bindings and clobber metadata.
     * Backend support is intentionally not provided yet.
     */
    inline std::optional< ast2_asm_callable > try_parse_asm_inline_callable(parsing_context& ctx)
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

        if (parsers::skip_keyword_if_is(pos, end, "CALLCONV"))
        {
            parsers::skip_whitespace_and_comments(pos, end);

            std::string callconv = parsers::parse_keyword(pos, end);

            output.calling_conv = std::move(callconv);

            parsers::skip_whitespace_and_comments(pos, end);
        }
        else
        {
            output.calling_conv = "CCALL";
        }

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw syntax_compilation_error("Expected '(' in ASM_INLINE_FUNCTION CALLABLE expression");
        }

        bool parsing_sections = false;
        while (true)
        {
            parsers::skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            if (parsing_sections || skip_symbol_if_is(pos, end, ";"))
            {
                parsing_sections = true;
                parsers::skip_whitespace_and_comments(pos, end);
                if (parsers::skip_keyword_if_is(pos, end, "RETURN"))
                {
                    parsers::skip_whitespace_and_comments(pos, end);
                    output.return_register_name = parse_register(pos, end);
                    parsers::skip_whitespace_and_comments(pos, end);
                    output.return_type = parsers::parse_type_symbol(trial);
                    parsers::skip_whitespace_and_comments(pos, end);

                    if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }

                    if (!skip_symbol_if_is(pos, end, ";"))
                    {
                        throw syntax_compilation_error("Expected ';' or ')' after ASM_INLINE_FUNCTION CALLABLE return declaration");
                    }
                    continue;
                }
                if (parsers::skip_keyword_if_is(pos, end, "CLOBBER"))
                {
                    parsers::skip_whitespace_and_comments(pos, end);
                    if (!skip_symbol_if_is(pos, end, "("))
                    {
                        throw syntax_compilation_error("Expected '(' after CLOBBER in ASM_INLINE_FUNCTION CALLABLE expression");
                    }

                    while (true)
                    {
                        parsers::skip_whitespace_and_comments(pos, end);
                        if (skip_symbol_if_is(pos, end, ")"))
                        {
                            break;
                        }

                        output.clobber.insert(parse_register(pos, end));
                        parsers::skip_whitespace_and_comments(pos, end);

                        if (skip_symbol_if_is(pos, end, ")"))
                        {
                            break;
                        }

                        if (!skip_symbol_if_is(pos, end, ","))
                        {
                            throw syntax_compilation_error("Expected ',' in ASM_INLINE_FUNCTION clobber list");
                        }
                    }

                    parsers::skip_whitespace_and_comments(pos, end);
                    if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }

                    if (!skip_symbol_if_is(pos, end, ";"))
                    {
                        throw syntax_compilation_error("Expected ';' or ')' after ASM_INLINE_FUNCTION clobber list");
                    }
                    continue;
                }

                throw syntax_compilation_error("Expected RETURN or CLOBBER after ';' in ASM_INLINE_FUNCTION CALLABLE expression");
            }

            auto register_name = parse_register(pos, end);

            parsers::skip_whitespace_and_comments(pos, end);

            auto input_type = parsers::parse_type_symbol(trial);

            output.args.push_back(ast2_argument_interface{.api_name = std::nullopt, .register_name = std::optional< std::string >(std::move(register_name)), .type = std::move(input_type)});

            parsers::skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                break;
            }

            if (skip_symbol_if_is(pos, end, ";"))
            {
                parsing_sections = true;
                continue;
            }

            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' in ASM_INLINE_FUNCTION CALLABLE expression");
            }
        }

        ctx.iter_pos = pos;
        return std::move(output);

    }

}

#endif //ASM_CALLABLE_HPP
