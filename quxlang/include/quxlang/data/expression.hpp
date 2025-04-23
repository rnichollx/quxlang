// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_HEADER_GUARD

#include "lookup_chain.hpp"
#include "numeric_literal.hpp"
#include "rpnx/string.hpp"

#include <quxlang/data/type_symbol.hpp>
#include <rpnx/compare.hpp>
#include <rpnx/variant.hpp>
#include <utility>
#include <vector>

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
    struct expression_multibind;

    struct expression_and;
    struct expression_or;
    struct expression_xor;
    struct expression_nand;
    struct expression_nor;
    struct expression_implies;
    struct expression_implied;

    struct expression_this_reference
    {
        RPNX_EMPTY_METADATA(expression_this_reference);
    };

    struct expression_dotreference;

    struct expression_thisdot_reference
    {
        std::string field_name;

        RPNX_MEMBER_METADATA(expression_thisdot_reference, field_name);
    };

    struct expression_quarrow
    {
        std::string field_name;

        RPNX_MEMBER_METADATA(expression_quarrow, field_name);
    };

    struct expression_lvalue_reference
    {
        std::string identifier;

        RPNX_MEMBER_METADATA(expression_lvalue_reference, identifier);
    };

    struct expression_symbol_reference
    {
        type_symbol symbol;

        RPNX_MEMBER_METADATA(expression_symbol_reference, symbol);
    };

    struct expression_binary
    {
        std::string operator_str;

        expression lhs;
        expression rhs;

        RPNX_MEMBER_METADATA(expression_binary, operator_str, lhs, rhs);
    };

    struct expression_unary_prefix
    {
        std::string operator_str;
        expression rhs;

        RPNX_MEMBER_METADATA(expression_unary_prefix, operator_str, rhs);
    };

    struct expression_unary_postfix
    {
        std::string operator_str;
        expression lhs;

        RPNX_MEMBER_METADATA(expression_unary_postfix, operator_str, lhs);
    };

    struct expression_dotreference
    {
        expression lhs;
        std::string field_name;

        RPNX_MEMBER_METADATA(expression_dotreference, lhs, field_name);
    };

    // Right arrow -> is used to get a reference from a pointer.
    struct expression_rightarrow
    {
        expression lhs;

        RPNX_MEMBER_METADATA(expression_rightarrow, lhs);
    };

    // Left arrow <- is used to get a pointer from a reference.
    struct expression_leftarrow
    {
        expression lhs;

        RPNX_MEMBER_METADATA(expression_leftarrow, lhs);
    };

    struct expression_multibind
    {
        std::string operator_str;
        expression lhs;
        std::vector< expression > bracketed;

        RPNX_MEMBER_METADATA(expression_multibind, operator_str, lhs, bracketed);
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
#include <quxlang/data/builtins.hpp>

#endif // QUXLANG_EXPRESSION_HEADER_GUARD