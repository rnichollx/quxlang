//
// Created by Ryan Nicholl on 2/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_AST_HEADER
#define RPNX_RYANSCRIPT1031_AST_HEADER
#include "value.hpp"
#include <string>
#include <variant>
#include <vector>

namespace rs1031
{
    struct ast_null_object
    {
        inline std::string to_string()
        {
            return "ast_null_object{}";
        }
    };

    struct ast_symbol_ref;
    struct ast_array_ref;
    struct ast_pointer_ref;
    struct ast_integral_keyword;

    struct ast_expr;
    struct ast_expr_load_symbol;
    struct ast_expr_assign;
    struct ast_expr_brackets;
    struct ast_expr_call;

    struct ast_expr : public value< std::variant< ast_null_object, ast_expr_load_symbol, ast_expr_assign > >
    {
    };

    struct ast_expr_brackets
    {
        ast_expr object;
        ast_expr index;
    };

    struct ast_expr_assign
    {
        ast_expr object;
        ast_expr value;
    };

    struct ast_statement
    {
    };

    struct ast_type_ref
    {
        value< std::variant< ast_null_object, ast_symbol_ref, ast_array_ref, ast_pointer_ref, ast_integral_keyword > > val;
        std::string to_string();
    };

    struct ast_symbol_ref
    {
        std::string name;
        inline std::string to_string()
        {
            return "ast_symbol_ref{ name: " + name + " }";
        }
    };

    struct ast_array_ref
    {
        ast_symbol_ref type;
        std::size_t size;

        inline std::string to_string()
        {
            return "ast_array_ref{ type: " + type.to_string() + ", size: " + std::to_string(size) + " }";
        }
    };

    struct ast_expr_load_symbol
    {
        ast_symbol_ref symbol;
    };

    struct ast_pointer_ref
    {
        ast_type_ref type;
        inline std::string to_string()
        {
            return "ast_pointer_ref{ type: " + type.to_string() + " }";
        }
    };

    struct ast_integral_keyword
    {
        bool signedness;
        int size;

        inline std::string to_string()
        {
            return "ast_integral_keyword{ signedness: " + std::to_string(signedness) + ", size: " + std::to_string(size) + " }";
        }
    };

    struct ast_member_variable
    {
        std::string name;
        ast_type_ref type;
    };

    struct ast_class
    {
        std::vector< ast_member_variable > member_variables;
    };

    struct ast_function_arg
    {
        std::string external_name;
        std::string name;
        ast_type_ref type;
        std::string to_string()
        {
            return "ast_arg{ external_name: " + external_name + ", name: " + name + ", type: " + type.to_string() + " }";
        }
    };

    struct ast_function
    {
        std::vector< ast_function_arg > args;
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
    };
    struct ast_top_function
    {
        std::string name;
        ast_function func;
    };

    inline std::string ast_type_ref::to_string()
    {
        return std::visit(
            [](auto&& arg) -> std::string {
                return arg.to_string();
            },
            val.get());
    }

} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_AST_HEADER
