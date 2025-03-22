// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_HEADER_GUARD
#include <optional>
#include <quxlang/data/expression.hpp>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/peek_symbol.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>

namespace quxlang::parsers
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
                {".<<", 7}, {".+>>", 7}, {".>>", 7}, {".@<", 7}, {".@>", 7} // clang-format on
            };

            std::string sym = peek_symbol(pos, end);
            auto it = operators_map.find(sym);
            if (it == operators_map.end())
                return false;

            skip_symbol_if_is(pos, end, sym);

            skip_whitespace_and_comments(pos, end);

            int priority = it->second;

            expression_binary new_expression;
            new_expression.operator_str = sym;

            expression* binding_point2 = operator_bindings[priority];
            new_expression.lhs = std::move(*binding_point2);
            *binding_point2 = quxlang::expression(new_expression);
            expression* binding_pointer = &as< expression_binary >(*binding_point2).rhs;
            for (int i = priority + 1; i < operator_bindings.size(); i++)
            {
                operator_bindings[i] = binding_pointer;
            }
            value_binding = &(as< expression_binary >(*binding_point2)).rhs;
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
        if (auto str = try_parse_string_literal(pos, end); str)
        {
            expression_string_literal str_lit;
            str_lit.value = str.value();
            *value_bind_point = str_lit;
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "TARGET"))
        {
            expression_target tg;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after TARGET");
            }
            tg.target = try_parse_string_literal(pos, end).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::logic_error("Expected ')' after TARGET(\"...\"");
            }
            *value_bind_point = tg;
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
        }
        else if (auto num_str = parse_int(pos, end); !num_str.empty())
        {
            expression_numeric_literal num;
            num.value = num_str;
            *value_bind_point = num;
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            skip_whitespace_and_comments(pos, end);
            expression_thisdot_reference thisdot;
            thisdot.field_name = parse_subentity(pos, end);
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
                throw std::logic_error("Expected ')'");
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
                throw std::logic_error("Expected binary operator to be followed by value");
            }
        }

    next_operator:
        remaining = std::string(pos, end);

        skip_whitespace_and_comments(pos, end);
        if (detail::binary_operator_v3(pos, end, bindings, value_bind_point))
        {
            goto next_value;
        }
        else if (std::optional< expression_call > call = try_parse_function_callsite_expression(pos, end); call)
        {
            expression* binding_point2 = bindings[bindings.size() - 1];
            call.value().callee = std::move(*binding_point2);
            *binding_point2 = quxlang::expression(call.value());
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            expression_dotreference dot;
            dot.field_name = parse_subentity(pos, end);
            dot.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = dot;
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, ":("))
        {
            // TODO: This part
            throw rpnx::unimplemented();
        }
        else if (skip_symbol_if_is(pos, end, "->"))
        {
            expression_rightarrow arrow;
            arrow.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(arrow);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "<-"))
        {
            expression_leftarrow arrow;
            arrow.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(arrow);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "["))
        {
            expression_brackets brkts;
            brkts.lhs = std::move(*bindings[bindings.size() - 1]);
            while (true)
            {
                brkts.bracketed.push_back(parse_expression(pos, end));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                else if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw std::logic_error("Expected ',' or ']' in brackets");
                }
            }
            *bindings[bindings.size() - 1] = std::move(brkts);
            goto next_operator;
        }
        else
        {
            return result;
        }

        throw rpnx::unimplemented();
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_EXPRESSION_HPP
