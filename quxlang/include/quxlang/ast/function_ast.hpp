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
        std::vector < function_delegate > delegates;
        std::optional<std::int64_t> priority;
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

            return std::tie(args, return_type, this_type, delegates, priority, body);
        }




        auto operator<(function_ast const & other) const
        {
            if (args < other.args) return true;
            if (other.args < args) return false;
            if (return_type < other.return_type) return true;
            if (other.return_type < return_type) return false;
            if (this_type < other.this_type) return true;
            if (other.this_type < this_type) return false;
            if (delegates < other.delegates) return true;
            if (other.delegates < delegates) return false;
            if (priority < other.priority) return true;
            if (other.priority < priority) return false;
            if (body < other.body) return true;
            if (other.body < body) return false;

        }
    };
} // namespace quxlang

#endif // QUXLANG_FUNCTION_AST_HEADER_GUARD
