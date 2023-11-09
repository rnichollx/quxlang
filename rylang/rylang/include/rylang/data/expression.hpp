//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_HEADER

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
    struct expression_multiply;

    struct expression_modulus;

    struct expression_divide;
    struct expression_equals;
    struct expression_not_equals;
    struct expression_binary;

    using expression = boost::variant< expression_this_reference, boost::recursive_wrapper< expression_add >, boost::recursive_wrapper< expression_addp >, boost::recursive_wrapper< expression_addw >,
                                       boost::recursive_wrapper< expression_call >, boost::recursive_wrapper< expression_lvalue_reference >, boost::recursive_wrapper< expression_multiply >,
                                       expression_thisdot_reference, boost::recursive_wrapper< expression_subtract >, boost::recursive_wrapper< expression_move_assign >,
                                       boost::recursive_wrapper< expression_copy_assign >, boost::recursive_wrapper< expression_and >, boost::recursive_wrapper< expression_or >,
                                       boost::recursive_wrapper< expression_xor >, boost::recursive_wrapper< expression_nand >, boost::recursive_wrapper< expression_nor >,
                                       boost::recursive_wrapper< expression_implies >, boost::recursive_wrapper< expression_implied >, boost::recursive_wrapper< expression_divide >,
                                       boost::recursive_wrapper< expression_modulus >, boost::recursive_wrapper< expression_dotreference >, boost::recursive_wrapper< expression_equals >,
                                       boost::recursive_wrapper< expression_not_equals >, boost::recursive_wrapper<expression_binary>, numeric_literal >;

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

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_HEADER
