// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_EXPRESSION_HEADER_GUARD
#define QUXLANG_DATA_EXPRESSION_HEADER_GUARD

#include "lookup_chain.hpp"
#include "numeric_literal.hpp"
#include "quxlang/macros.hpp"

#include <quxlang/data/type_symbol.hpp>
#include <compare>
#include <rpnx/compare.hpp>
#include <rpnx/variant.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

RPNX_ENUM(quxlang, integral_qualifier, std::uint8_t, none, signed_, unsigned_);

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

    struct expression_static_choose;
    struct expression_snapshot;



    struct expression_this_reference
    {
        QUXLANG_WITH_SOURCE_LOCATION_EMPTY_METADATA(expression_this_reference);
    };

    struct expression_dotreference;

    struct expression_thisdot_reference
    {
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_thisdot_reference, field_name);
    };

    struct expression_quarrow
    {
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_quarrow, field_name);
    };



    struct expression_symbol_reference
    {
        type_symbol symbol;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_symbol_reference, symbol);
    };

    struct expression_target
    {
        std::string target;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_target, target);
    };


    struct expression_binary
    {
        std::string operator_str;

        expression lhs;
        expression rhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_binary, operator_str, lhs, rhs);
    };

    struct expression_unary_prefix
    {
        std::string operator_str;
        expression rhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_unary_prefix, operator_str, rhs);
    };

    struct expression_unary_postfix
    {
        std::string operator_str;
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_unary_postfix, operator_str, lhs);
    };

    struct expression_dotreference
    {
        expression lhs;
        std::string field_name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_dotreference, lhs, field_name);
    };

    // Right arrow -> is used to get a reference from a pointer.
    struct expression_rightarrow
    {
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_rightarrow, lhs);
    };

    // Left arrow <- is used to get a pointer from a reference.
    struct expression_leftarrow
    {
        expression lhs;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_leftarrow, lhs);
    };

    struct expression_multibind
    {
        std::string operator_str;
        expression lhs;
        std::vector< expression > bracketed;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_multibind, operator_str, lhs, bracketed);
    };

    struct expression_lvalue_reference
    {
        lookup_chain chain;
    };

    struct expression_equals
    {
        static constexpr const char* name = "equals";
        static constexpr const char* symbol = "==";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_equals const& other) const = default;
    };

    struct expression_not_equals
    {
        static constexpr const char* name = "not_equals";
        static constexpr const char* symbol = "!=";
        static constexpr const int priority = 2;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_not_equals const& other) const = default;
    };

    struct expression_multiply
    {
        static constexpr const char* const symbol = "*";
        static constexpr const char* const name = "multiply";
        static constexpr const int priority = 5;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_multiply const& other) const = default;
    };

    struct expression_divide
    {
        static constexpr const char* const symbol = "/";
        static constexpr const char* const name = "divide";
        static constexpr const int priority = 3;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_divide const& other) const = default;
    };

    struct expression_modulus
    {
        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_modulus const& other) const = default;
    };

    struct expression_subtract
    {
        static constexpr const char* name = "subtract";
        static constexpr const char* symbol = "-";
        static constexpr const int priority = 4;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_subtract const& other) const = default;
    };

    struct expression_move_assign
    {
        static constexpr const char* name = "move_assign";
        static constexpr const char* symbol = ":<";
        static constexpr const int priority = 0;

        expression lhs;
        expression rhs;

        std::strong_ordering operator<=>(expression_move_assign const& other) const = default;
    };

    struct expression_value_keyword
    {
        std::string keyword;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_value_keyword, keyword);
    };

    struct expression_arg
    {
        std::optional< std::string > name;
        expression value;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_arg, name, value);
    };

    struct call_initializer
    {
        std::vector< expression_arg > args;
        RPNX_MEMBER_METADATA(call_initializer, args);
    };

    // An expression like ` :[ a, b, c ] ` for array initialization
    // or ` :[ a, b, c, ... ]` for array initialization with default value
    struct array_initializer
    {
        std::vector< expression > args;
        std::optional< expression > default_initalization;
        RPNX_MEMBER_METADATA(array_initializer, args, default_initalization);
    };

    struct assignment_initializer
    {
        expression expr;
        RPNX_MEMBER_METADATA(assignment_initializer, expr);
    };



    struct expression_call
    {
        expression callee;
        std::vector< expression_arg > args;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_call, callee, args);
    };

    struct expression_bits
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_bits, of_type);
    };

    struct expression_sizeof
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_sizeof, of_type);
    };

    struct expression_is_integral
    {
        type_symbol of_type;
        integral_qualifier qualifier = integral_qualifier::none;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_is_integral, of_type, qualifier);
    };

    struct expression_same_types
    {
        type_symbol lhs_type;
        type_symbol rhs_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_same_types, lhs_type, rhs_type);
    };

    struct expression_is_signed
    {
        type_symbol of_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_is_signed, of_type);
    };


    struct expression_typecast
    {
        expression expr;
        type_symbol to_type;
        std::optional< std::string > keyword;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_typecast, expr, to_type, keyword);
    };

    struct expression_pun
    {
        expression value;
        type_symbol as_type;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_pun, value, as_type);
    };

    struct expression_place
    {
        expression at;
        type_symbol type;
        std::optional< expression > assign_init;
        std::vector< expression_arg > args;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_place, at, type, assign_init, args);
    };

    struct expression_static_choose
    {
        expression condition;
        expression true_expr;
        expression false_expr;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_static_choose, condition, true_expr, false_expr);
    };

    struct expression_snapshot
    {
        /// Visible function-local static name to freeze for this runtime expression.
        std::string name;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_snapshot, name);
    };

    struct expression_choose
    {
        expression condition;
        expression true_expr;
        expression false_expr;

        QUXLANG_WITH_SOURCE_LOCATION_METADATA(expression_choose, condition, true_expr, false_expr);
    };

    struct delegate
    {
        // The name of the delegate
        std::string name;

        // TODO: Delegates should be able to refer to members by complex symbols

        // Expression arguments in a delegate call
        std::vector< expression_arg > args;

        RPNX_MEMBER_METADATA(delegate, name, args);
    };



} // namespace quxlang

namespace quxlang
{
    inline std::optional< source_location > get_location(expression const& expr)
    {
        return rpnx::apply_visitor< std::optional< source_location > >(expr, [](auto const& value) { return value.location; });
    }

    inline void set_location(expression& expr, std::optional< source_location > location)
    {
        rpnx::apply_visitor<void>(expr, [&](auto& value) { value.location = location; });
    }
}

#endif // QUXLANG_EXPRESSION_HEADER_GUARD
