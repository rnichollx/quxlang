// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FUNCTION_ARGS_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FUNCTION_ARGS_HEADER_GUARD
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::vector< ast2_function_parameter > parse_function_args(It& pos, It end)
    {
        std::vector< ast2_function_parameter > result;

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::logic_error("Expected '('");
        }

        skip_whitespace_and_comments(pos, end);
        // expect_more(pos, end);

        if (skip_symbol_if_is(pos, end, ")"))
        {
            return result;
        }

    get_arg:

        skip_whitespace_and_comments(pos, end);

        std::string remaining = std::string(pos, end);
        ast2_function_parameter arg;

        if (skip_symbol_if_is(pos, end, "@"))
        {
            arg.api_name = parse_argument_name(pos, end);
            if (arg.api_name->empty())
            {
                throw std::logic_error("Expected identifier after '@' ");
            }

            if (skip_symbol_if_is(pos, end, ":"))
            {
                arg.name = parse_identifier(pos, end);
            }

            if (!skip_whitespace(pos, end))
            {
                throw std::logic_error("Expected whitespace after external parameter name");
            }

        }
        else if (skip_symbol_if_is(pos, end, "%"))
        {
            if (!skip_keyword_if_is(pos, end, "IGNORED"))
            {
                arg.name = parse_identifier(pos, end);
                if (arg.name->empty())
                {
                    throw std::logic_error("Expected identifier after '%', use '%IGNORED' to accept an argument without a name");
                }
            }


        }
        else
        {
            throw std::logic_error("Expected positional '%' or named '@' argument");
        }


        type_symbol& arg_type = arg.type;



        skip_whitespace_and_comments(pos, end);

        std::string remaining_after_name = std::string(pos, end);

        arg_type = parse_type_symbol(pos, end);

        result.push_back(std::move(arg));

        skip_whitespace_and_comments(pos, end);

        if (skip_symbol_if_is(pos, end, ")"))
        {
            return result;
        }
        else if (skip_symbol_if_is(pos, end, ","))
        {
            goto get_arg;
        }
        else
        {
            throw std::logic_error("Expected ',' or ')'");
        }

        // TODO: Check for duplicate argument names here?
    }

    inline std::vector< ast2_function_parameter > parse_function_args(std::string str)
    {
        auto it = str.begin();
        return parse_function_args(it, str.end());
    }
} // namespace quxlang::parsers

#endif // PARSE_FUNCTION_ARGS_HPP
