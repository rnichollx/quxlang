//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_EXPRESSION_HEADER_GUARD
#define QUXLANG_EXPRESSION_HEADER_GUARD

#include "lookup_chain.hpp"
#include "numeric_literal.hpp"
#include "rpnx/string.hpp"

#include <boost/variant.hpp>
#include <quxlang/data/qualified_symbol_reference.hpp>
#include <utility>
#include <vector>
#include <rpnx/variant.hpp>
#include <rpnx/compare.hpp>


namespace quxlang
{

    struct expression_add;
    struct expression_subtract;
    struct expression_addp;
    struct expression_addw;
    struct expression_call;
    struct expression_lvalue_reference;
    struct expression_copy_assign;
    struct expression_move_assign;
    struct expression_parenthesis;

    struct expression_and;
    struct expression_or;
    struct expression_xor;
    struct expression_nand;
    struct expression_nor;
    struct expression_implies;
    struct expression_implied;

    struct expression_this_reference
    {
        std::strong_ordering operator<=>(const expression_this_reference& other) const = default;
    };

    struct expression_dotreference;

    struct expression_thisdot_reference
    {
        std::string field_name;
        std::strong_ordering operator<=>(const expression_thisdot_reference& other) const = default;
    };

    struct expression_quarrow
    {
        std::string field_name;
        auto operator<=>(const expression_quarrow&) const;
    };

    struct expression_lvalue_reference
    {
        std::string identifier;

        std::strong_ordering operator<=>(const expression_lvalue_reference& other) const = default;
    };

    struct expression_symbol_reference
    {
        type_symbol symbol;

        std::strong_ordering operator<=>(const expression_symbol_reference& other) const = default;
    };

    struct expression_multiply;

    struct expression_modulus;

    struct expression_divide;
    struct expression_equals;
    struct expression_not_equals;
    struct expression_binary;
    using expression = rpnx::variant< expression_this_reference, expression_call, expression_symbol_reference, expression_thisdot_reference, expression_dotreference, expression_binary, numeric_literal >;

    struct expression_binary
    {
        rpnx::string operator_str;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_binary& other) const
        {
            return compare(operator_str, other.operator_str, lhs, other.lhs, rhs, other.rhs);
        }
    };

} // namespace quxlang

#include "quxlang/data/expression_add.hpp"
#include "quxlang/data/expression_bool.hpp"
#include "quxlang/data/expression_call.hpp"
#include "quxlang/data/expression_copy_assign.hpp"
#include "quxlang/data/expression_dotreference.hpp"
#include "quxlang/data/expression_equals.hpp"
#include "quxlang/data/expression_move_assign.hpp"
#include "quxlang/data/expression_multiply.hpp"
#include "quxlang/data/expression_subtract.hpp"

#endif // QUXLANG_EXPRESSION_HEADER_GUARD