//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER

#include <optional>
#include <string>

#include "function_arg_ast.hpp"
#include "rylang/data/function_block.hpp"
#include "rylang/data/type_reference.hpp"

namespace rylang
{
    struct function_ast
    {
        std::vector< function_arg_ast > args;
        std::optional< type_reference > return_type;
        function_block body;

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

        auto tie() const
        {
            return std::tie(args, return_type, body);
        }


        // auto operator<=>(function_ast const&) const = default;
        bool operator<(function_ast const& other) const
        {
            return tie() < other.tie();
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_AST_HEADER
