//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_EXPRESSION_HEADER_GUARD
#define RYLANG_EXPRESSION_HEADER_GUARD

#include "lookup_chain.hpp"
#include "numeric_literal.hpp"
#include <boost/variant.hpp>
#include <utility>
#include <vector>

namespace rylang
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

    using expression = boost::variant< expression_this_reference, boost::recursive_wrapper< expression_call >, boost::recursive_wrapper< expression_symbol_reference >,  expression_thisdot_reference, boost::recursive_wrapper< expression_dotreference >, boost::recursive_wrapper< expression_binary >, numeric_literal >;

    struct expression_binary
    {
        std::string operator_str;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(const expression_binary& other) const = default;
    };

} // namespace rylang

#include "rylang/data/expression_add.hpp"
#include "rylang/data/expression_bool.hpp"
#include "rylang/data/expression_call.hpp"
#include "rylang/data/expression_copy_assign.hpp"
#include "rylang/data/expression_dotreference.hpp"
#include "rylang/data/expression_equals.hpp"
#include "rylang/data/expression_move_assign.hpp"
#include "rylang/data/expression_multiply.hpp"
#include "rylang/data/expression_subtract.hpp"

#endif // RYLANG_EXPRESSION_HEADER_GUARD
