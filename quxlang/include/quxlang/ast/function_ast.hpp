//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_FUNCTION_AST_HEADER_GUARD
#define QUXLANG_FUNCTION_AST_HEADER_GUARD

#include <optional>
#include <string>

#include "function_arg_ast.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/function_block.hpp"
#include "quxlang/data/function_delegate.hpp"
#include <cinttypes>

namespace quxlang
{
    struct function_ast
    {
        std::vector< function_arg_ast > args;
        std::optional< type_symbol > return_type;
        std::optional< type_symbol > this_type;
        std::vector< function_delegate > delegates;
        std::optional< std::int64_t > priority;
        function_block body;

        std::strong_ordering operator<=>(const function_ast& other) const
        {
            return rpnx::compare(args, other.args, return_type, other.return_type, this_type, other.this_type, delegates, other.delegates, priority, other.priority, body, other.body);
        }

        std::string to_string() const
        {
            std::string result = "ast_function{ args: [";
            auto next_it = args.begin();
            for (auto it = args.begin(); it != args.end(); it = next_it)
            {
                next_it = it;
                ++next_it;
                auto& arg = *it;
                result += arg.to_string();
                if (next_it != args.end())
                {
                    result += ", ";
                }
            }
            result += "] }";
            return result;
        }

    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_AST_HEADER_GUARD