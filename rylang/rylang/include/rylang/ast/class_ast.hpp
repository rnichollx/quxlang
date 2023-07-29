//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_AST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_AST_HEADER

#include "member_variable_ast.hpp"
#include <vector>

namespace rylang
{
    struct class_ast
    {
        std::vector< member_variable_ast > member_variables;
        inline std::string to_string()
        {
            std::string result = "ast_class{ member_variables: [";
            auto next_it = member_variables.begin();
            for (auto it = member_variables.begin(); it != member_variables.end(); it = next_it)
            {
                next_it = it;
                ++next_it;
                auto& arg = *it;
                result += arg.to_string();
                if (next_it != member_variables.end())
                {
                    result += ", ";
                }
            }
            result += "] }";
            return result;
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_AST_HEADER
