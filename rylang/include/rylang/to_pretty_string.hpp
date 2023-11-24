//
// Created by Ryan Nicholl on 11/23/23.
//

#ifndef TO_PRETTY_STRING_HPP
#define TO_PRETTY_STRING_HPP

#include "data/expression.hpp"
#include "data/function_block.hpp"
#include "data/qualified_symbol_reference.hpp"
#include "data/vm_executable_unit.hpp"
#include "data/vm_expression.hpp"
#include <string>

namespace rylang
{
    class to_pretty_string_visitor : public boost::static_visitor< std::string >
    {
    private:
        std::size_t current_indent = 0;
        std::size_t indent_step = 4;

        std::string indent_string() const
        {
            for (std::size_t i = 0; i < current_indent * indent_step; ++i)
            {
                result += ' ';
            }
        }

        std::string operator()(vm_expr_primitive_binary_op const& op)
        {
            std::string result;
            result += "vm_expr_primitive_binary_op{ op: ";
            result += op.oper;
            result += "\n";
            ++current_indent;
            result += indent_string();
            result += "left: ";
            result += boost::apply_visitor(*this, op.left);
            result += indent_string();
            result += "right: ";
            result += boost::apply_visitor(*this, op.right);
            result += current_indent() "}\n;";
            --current_indent;
            return result;
        }

        std::string operator()(vm_expr_access_field const& op)
        {
            std::string result;
            result += "vm_expr_access_field{ field: ";
            current_indent++;
            result += op.offset;
            result += "\n";
            result += indent_string();
            result += "base: ";
            result += boost::apply_visitor(*this, op.base);
            result += "type: ";
            result += op.type;
            current_indent--;
            result += indent_string() + "}\n;";
            return result;
        }


    };

    inline std::string to_pretty_string(expression expr)
    {
        to_pretty_string_visitor visitor;
        return boost::apply_visitor(visitor, expr);
    }
}

#endif // TO_PRETTY_STRING_HPP
