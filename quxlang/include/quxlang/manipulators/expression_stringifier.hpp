// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_EXPRESSION_STRINGIFIER_HEADER_GUARD
#define QUXLANG_MANIPULATORS_EXPRESSION_STRINGIFIER_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include "quxlang/data/expression_numeric_literal.hpp"
#include "quxlang/data/numeric_literal.hpp"
#include <string>

namespace quxlang
{

    std::string to_string(expression const& expr);

    struct expression_stringifier
    {
        expression_stringifier()
        {
        }

        std::string operator()(expression_unary_postfix const& be) const
        {
            return "(" + to_string(be.lhs) + " " + be.operator_str + ")";
        }

        std::string operator()(expression_unary_prefix const& be) const
        {
            return "(" + be.operator_str + " " + to_string(be.rhs) + ")";
        }



        std::string operator()(expression_brackets const& brkts) const
        {
            std::string result;
            result += to_string(brkts.lhs);
            result += " [ ";
            for (std::size_t i = 0; i < brkts.bracketed.size(); i++)
            {
                result += to_string(brkts.bracketed[i]);
                if (i != brkts.bracketed.size() - 1)
                    result += " , ";
            }
            result += " ]";
            return result;
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
                auto arg = expr.args[i].value;
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

        std::string operator()(expression_target const& expr) const
        {
            return "TARGET" + expr.target;
        }

        std::string operator()(expression_sizeof const& expr) const
        {
            return "SIZEOF(" + to_string(expr.what) + ")";
        }

        std::string operator()(expression_string_literal const& expr) const
        {
            // TODO: Escape
            return "\"" + expr.value + "\"";
        }

        std::string operator()(expression_leftarrow const& expr) const
        {
            std::string output = "(" + to_string(expr.lhs) + " <- )";
            return output;
        }

        std::string operator()(expression_rightarrow const& expr) const
        {
            std::string output = "(" + to_string(expr.lhs) + " -> )";
            return output;
        }
    };

    inline std::string to_string(expression const& expr)
    {
        auto str = rpnx::apply_visitor< std::string >(expression_stringifier{}, expr);

        // TODO: replace all "  " with " "
        return str;
    }
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_STRINGIFIER_HEADER_GUARD
