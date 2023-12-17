//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef PARSE_FUNCTION_ARGS_HPP
#define PARSE_FUNCTION_ARGS_HPP
#include <rylang/ast2/ast2_function_arg.hpp>
#include <rylang/data/call_parameter_information.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/try_parse_type_symbol.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::vector< ast2_function_arg > parse_function_args(It& pos, It end)
    {
        std::vector< ast2_function_arg > result;

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
        ast2_function_arg arg;

        if (skip_symbol_if_is(pos, end, "@"))
        {
            arg.api_name = get_skip_identifier(pos, end);
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

        arg_name = get_skip_identifier(pos, end);
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

    std::vector< ast2_function_arg > parse_function_args(std::string str)
    {
        auto it = str.begin();
        return parse_function_args(it, str.end());
    }
} // namespace rylang::parsers

#endif // PARSE_FUNCTION_ARGS_HPP
