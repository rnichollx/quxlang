/**
 * @file parse_function_block.hpp
 * @brief Contains parsers for function block constructs in Quxlang.
 *
 * This file defines functions to parse function blocks enclosed in braces '{' and '}'.
 * It utilizes helper functions to skip whitespace and comments and converts the 
 * input into a structured function_block.
 */

// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#include <quxlang/data/function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_statement.hpp>
#include <quxlang/parsers/fwd.hpp> // added forward declarations

namespace quxlang::parsers
{
    /**
     * @brief Attempts to parse a function block from the given iterator range.
     *
     * Skips whitespace and comments, then checks for the opening '{'. Continues to parse
     * function statements until the closing '}' is encountered.
     *
     * @tparam It Iterator type.
     * @param pos Iterator referencing the current position.
     * @param end End iterator marking the limit.
     * @return Optional function_block if successfully parsed; std::nullopt if the opening '{' is missing.
     * @throw std::logic_error When the closing '}' is not found after parsing the statements.
     */
    template < typename It >
    std::optional<function_block> try_parse_function_block(It& pos, It end)
    {
        function_block body;
        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
           return std::nullopt;// throw std::logic_error("Expected '{'");
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, "}"))
        {
            // end of function body
            return body;
        }

        std::optional< function_statement > statement;

        while ((statement = try_parse_statement(pos, end)))
        {
            body.statements.push_back(std::move(statement.value()));
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "}"))
            {
                // end of function body
                return body;
            }
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, "}"))
        {
            // end of function body
            return body;
        }
        auto remaining = std::string(pos, end);
        throw std::logic_error("Expected '}' or statement");
    }

    /**
     * @brief Parses a function block from the input iterator range.
     *
     * Wrapper around try_parse_function_block. Throws an exception if a function block cannot be parsed.
     *
     * @tparam It Iterator type.
     * @param pos Iterator referencing the current position.
     * @param end End iterator marking the limit.
     * @return A function_block representing the parsed function block.
     * @throw std::logic_error When the function block is not successfully parsed.
     */
    template <typename It>
    function_block parse_function_block(It& pos, It end)
    {
        auto fb = try_parse_function_block(pos, end);
        if (fb)
        {
            return *fb;
        }
        throw std::logic_error("Expected a function block");
    }

} // namespace quxlang::parsers

#endif // PARSE_FUNCTION_BODY_HPP

