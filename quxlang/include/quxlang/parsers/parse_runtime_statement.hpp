// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_PARSERS_PARSE_RUNTIME_STATEMENT_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_RUNTIME_STATEMENT_HEADER_GUARD

#include <quxlang/data/function_statement.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>

namespace quxlang::parsers
{
    template < typename It >
    function_runtime_statement parse_runtime_statement(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "RUNTIME"))
        {
            throw std::logic_error("Expected 'RUNTIME'");
        }

        skip_whitespace_and_comments(pos, end);

        // Only allow CONSTEXPR and NATIVE conditions for now
        if (skip_keyword_if_is(pos, end, "CONSTEXPR"))
        {
            function_runtime_statement st;
            st.condition = runtime_condition::CONSTEXPR;
            skip_whitespace_and_comments(pos, end);
            st.then_block = parse_function_block(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "ELSE"))
            {
                skip_whitespace_and_comments(pos, end);
                st.else_block = parse_function_block(pos, end);
            }
            return st;
        }
        else if (skip_keyword_if_is(pos, end, "NATIVE"))
        {
            function_runtime_statement st;
            st.condition = runtime_condition::NATIVE;
            skip_whitespace_and_comments(pos, end);
            st.then_block = parse_function_block(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "ELSE"))
            {
                skip_whitespace_and_comments(pos, end);
                st.else_block = parse_function_block(pos, end);
            }
            return st;
        }
        else
        {
            throw std::logic_error("Expected runtime condition 'CONSTEXPR' or 'NATIVE' after 'RUNTIME'");
        }
    }
}

#endif // QUXLANG_PARSERS_PARSE_RUNTIME_STATEMENT_HEADER_GUARD
