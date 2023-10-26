//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_STRINGIFIER_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_STRINGIFIER_HEADER

#include "rylang/data/expression.hpp"
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

        std::string operator()(...) const
        {
            return "???";
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

        std::string operator()(expression_thisdot_reference const& expr) const
        {
            return "." + expr.field_name;
        }

        std::string operator()(expression_copy_assign const& expr) const
        {
            return "(" + to_string(expr.lhs) + " := " + to_string(expr.rhs) + ")";
        }
    };

    inline std::string to_string(expression const& expr)
    {
        return boost::apply_visitor(expression_stringifier{}, expr);
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_STRINGIFIER_HEADER
