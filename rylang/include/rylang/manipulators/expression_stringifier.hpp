//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_EXPRESSION_STRINGIFIER_HEADER_GUARD
#define RYLANG_EXPRESSION_STRINGIFIER_HEADER_GUARD

#include "rylang/data/expression.hpp"
#include "rylang/data/expression_numeric_literal.hpp"
#include <string>

namespace rylang
{

    std::string to_string(expression const& expr);

    struct expression_stringifier : public boost::static_visitor< std::string >
    {
        expression_stringifier()
        {
        }
        std::string operator()(expression_add const& expr) const
        {
            return "(" + to_string(expr.lhs) + " + " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_numeric_literal const& expr) const
        {
            return expr.value;
        }

        std::string operator()(expression_dotreference const& expr) const
        {
            return "(" + to_string(expr.lhs) + "." + expr.field_name + ")";
        }

        std::string operator()(expression_call const& expr) const
        {
            std::string result = "(" + to_string(expr.callee) + "(";
            for (int i = 0; i < expr.args.size(); i++)
            {
                auto arg = expr.args[i];
                result += to_string(arg);
                if (i != expr.args.size() - 1)
                    result += ", ";
            }
            result += "))";
            return result;
        }

        std::string operator()(expression_lvalue_reference const& lvalue) const
        {
            return lvalue.identifier;
        }

        std::string operator()(expression_multiply const& expr) const
        {
            return "(" + to_string(expr.lhs) + " * " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_xor const& expr) const
        {
            return "(" + to_string(expr.lhs) + " ^^ " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_and const& expr) const
        {
            return "(" + to_string(expr.lhs) + " && " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_copy_assign const& expr) const
        {
            return "(" + to_string(expr.lhs) + " := " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_binary const& expr) const
        {
            return "(" + to_string(expr.lhs) + " " + expr.operator_str + " " + to_string(expr.rhs) + ")";
        }

        std::string operator()(expression_this_reference const& expr) const
        {
            return "THIS";
        }

        std::string operator()(expression_thisdot_reference const& expr) const
        {
            return "." + expr.field_name;
        }

        std::string operator()(expression_symbol_reference const& expr) const
        {
            return to_string(expr.symbol);
        }

        std::string operator()(numeric_literal const& expr) const
        {
            return expr.value;
        }
    };

    inline std::string to_string(expression const& expr)
    {
        return boost::apply_visitor(expression_stringifier{}, expr);
    }
} // namespace rylang

#endif // RYLANG_EXPRESSION_STRINGIFIER_HEADER_GUARD
