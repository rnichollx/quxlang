//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_EXPRESSION_HPP
#define TRY_PARSE_EXPRESSION_HPP
#include <optional>
#include <rylang/data/expression.hpp>
#include <rylang/data/qualified_symbol_reference.hpp>
#include <rylang/parsers/parse_int.hpp>
#include <rylang/parsers/parse_whitespace_and_comments.hpp>
#include <rylang/parsers/try_parse_type_symbol.hpp>

#include <rylang/parsers/try_parse_function_callsite_expression.hpp>

namespace rylang::parsers
{
    template < typename It >
    expression parse_expression(It& pos, It end);

    namespace detail
    {
        template < typename It >
        bool binary_operator_v3(It& pos, It end, std::vector< expression* >& operator_bindings, expression*& value_binding)
        {
            static std::map< std::string, int > operators_map = {
                // clang-format off

                // Assignment operators
                {":=", 0}, {":<", 0},

                // Logical operators
                {"&&", 1}, // and
                {"!&", 1}, // nand
                {"^^", 1}, // xor
                {"!|", 1}, // nor
                {"||", 1}, // or
                {"^>", 1}, // implies
                {"^<", 1}, // implied
                {"!^", 1}, // equilvalent/nxor

                // Comparison operators
                {"==", 2}, {"!=", 2}, {"<=", 2}, {">=", 2}, {"<", 2}, {">", 2},

                // Division and modulus
                {"/", 3}, {"%", 3},

                // Addition and subtraction
                {"+", 4}, {"-", 4}, // regular
                // TODO:
                //{"+~", 4}, {"-~", 4}, // wrap-around
                //{"+!", 4}, {"-!", 4}, // undefined overflow

                // Multiplication
                {"*", 5},

                // Exponenciation
                {"^", 6},

                // Bitwise operators, same as logical except begin with a dot.
                {".&&", 7}, // bitwise and
                {".!&", 7}, // bitwise nand
                {".^^", 7}, // bitwise xor
                {".!|", 7}, // bitwise nor
                {".||", 7}, // bitwise or
                {".^>", 7}, // bitwise implies
                {".^<", 7}, // bitwise implied
                {".!^", 7}, // bitwise equilvalent

                // plus some additional shift and rotation operators
                {".<<", 7}, {".+>>", 7}, {".>>", 7}, {".@<", 7}, {".@>", 7}
                // clang-format on
            };

            std::string sym = get_symbol(pos, end);
            auto it = operators_map.find(sym);
            if (it == operators_map.end())
                return false;

            skip_symbol_if_is(pos, end, sym);

            skip_wsc(pos, end);

            int priority = it->second;

            expression_binary new_expression;
            new_expression.operator_str = sym;

            expression* binding_point2 = operator_bindings[priority];
            new_expression.lhs = std::move(*binding_point2);
            *binding_point2 = rylang::expression(new_expression);
            expression* binding_pointer = &boost::get< expression_binary >(*binding_point2).rhs;
            for (int i = priority + 1; i < operator_bindings.size(); i++)
            {
                operator_bindings[i] = binding_pointer;
            }
            value_binding = &(boost::get< expression_binary >(*binding_point2)).rhs;
            return true;
        }
    } // namespace detail

    template < typename It >
    std::optional< expression > try_parse_expression(It& pos, It end)
    {
        skip_whitespace_and_comments(pos, end);

        std::string remaining{pos, end};

        expression result;
        std::vector< expression* > bindings;

        bindings.resize(9);

        for (auto& binding : bindings)
        {
            binding = &result;
        }

        expression* value_bind_point = &result;
        bool have_anything = false;

    next_value:

        remaining = std::string(pos, end);
        std::optional< type_symbol > sym;
        skip_whitespace_and_comments(pos, end);
        if (auto num_str = parse_int(pos, end); !num_str.empty())
        {
            numeric_literal num;
            num.value = num_str;
            *value_bind_point = num;
            have_anything = true;
            skip_wsc(pos, end);
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            skip_wsc(pos, end);
            expression_thisdot_reference thisdot;
            thisdot.field_name = get_skip_identifier(pos, end);
            *value_bind_point = thisdot;
            have_anything = true;
        }
        else if (auto sym = try_parse_type_symbol(pos, end); sym)
        {
            assert(sym.has_value());

            expression_symbol_reference lvalue;
            lvalue.symbol = sym.value();

            *value_bind_point = lvalue;
            have_anything = true;
        }
        else if (skip_symbol_if_is(pos, end, "("))
        {

            expression parenthesis;
            parenthesis = parse_expression(pos, end);
            *value_bind_point = parenthesis;
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::runtime_error("Expected ')'");
            }
        }
        else
        {
            if (!have_anything)
            {
                return std::nullopt;
            }
            else
            {
                throw std::runtime_error("Expected binary operator to be followed by value");
            }
        }

    next_operator:
        remaining = std::string(pos, end);

        skip_wsc(pos, end);
        if (detail::binary_operator_v3(pos, end, bindings, value_bind_point))
        {
            goto next_value;
        }
        else if (std::optional< expression_call > call = try_parse_function_callsite_expression(pos, end); call)
        {
            expression* binding_point2 = bindings[bindings.size() - 1];
            call.value().callee = std::move(*binding_point2);
            *binding_point2 = rylang::expression(call.value());
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            expression_dotreference dot;
            dot.field_name = get_skip_identifier(pos, end);
            dot.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = dot;
            goto next_operator;
        }
        else
        {
            return result;
        }
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_EXPRESSION_HPP
