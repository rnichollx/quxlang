//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_FUNCTION_ARGS_HPP
#define PARSE_FUNCTION_ARGS_HPP
#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/data/call_parameter_information.hpp>
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
            throw std::runtime_error("Expected '('");
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
            arg.api_name = parse_identifier(pos, end);
            if (arg.api_name->empty())
            {
                throw std::runtime_error("Expected identifier after '@' ");
            }

            if (!skip_whitespace(pos, end))
            {
                throw std::runtime_error("Expected whitespace after external parameter name");
            }
        }

        if (!skip_symbol_if_is(pos, end, "%"))
        {
            throw std::runtime_error("Expected '%' or ')'");
        }

        std::string& arg_name = arg.name;
        type_symbol& arg_type = arg.type;

        arg_name = parse_identifier(pos, end);
        if (arg_name.empty())
        {
            throw std::runtime_error("Expected identifier");
        }

        skip_whitespace_and_comments(pos, end);

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
            throw std::runtime_error("Expected ',' or ')'");
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
