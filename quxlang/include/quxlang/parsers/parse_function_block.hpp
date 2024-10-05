//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FUNCTION_BLOCK_HEADER_GUARD
#include <quxlang/data/function_block.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_statement.hpp>

namespace quxlang::parsers
{
    template < typename It >
    function_block parse_function_block(It& pos, It end)
    {
        function_block body;
        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, "{"))
        {
            throw std::logic_error("Expected '{'");
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

} // namespace quxlang::parsers

#endif // PARSE_FUNCTION_BODY_HPP
