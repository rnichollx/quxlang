//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER

#include <string>

#include "function_arg_ast.hpp"

namespace rylang
{
    struct function_ast
    {
        std::vector< function_arg_ast > args;
        std::string to_string()
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
        //auto operator<=>(function_ast const&) const = default;
        bool operator < (function_ast const& other) const
        {
            return args < other.args;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER
