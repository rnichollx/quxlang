// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_EXPRESSION_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/lang/lang.hpp"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/function_statement.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/operators.hpp>
#include <quxlang/parsers/iter_parse_number.hpp>
#include <quxlang/parsers/parse_int.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/peek_symbol.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_subentity.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

#include <quxlang/parsers/string_literal.hpp>
#include <quxlang/parsers/try_parse_function_callsite_expression.hpp>
#include <quxlang/parsers/fwd.hpp>

namespace quxlang::parsers
{
    inline std::optional< expression > try_parse_expression(parsing_context& ctx);

    namespace detail
    {
        inline std::optional< expression > try_parse_expression_impl(parsing_context& ctx);

        inline expression parse_expression_impl(parsing_context& ctx)
        {
            auto& pos = ctx.iter_pos;
            auto end = ctx.iter_end;
            skip_whitespace_and_comments(pos, end);
            auto expr_begin = pos;
            if (auto output = try_parse_expression_impl(ctx); output)
            {
                auto result = std::move(*output);
                set_location(result, ctx.get_location_optional(parse_iterator(expr_begin), parse_iterator(pos)));
                return result;
            }
            throw syntax_compilation_error("Expected expression");
        }

        inline std::vector< lambda_capture > parse_lambda_captures(parsing_context& ctx)
        {
            auto& pos = ctx.iter_pos;
            auto end = ctx.iter_end;
            std::vector< lambda_capture > captures;
            if (!skip_symbol_if_is(pos, end, "["))
            {
                throw syntax_compilation_error("Expected '['");
            }
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "]"))
            {
                return captures;
            }
            while (true)
            {
                auto capture_begin = pos;
                lambda_capture capture;
                capture.mode = skip_symbol_if_is(pos, end, "=") ? lambda_capture_mode::value : lambda_capture_mode::reference;
                skip_whitespace_and_comments(pos, end);
                capture.name = parse_identifier(pos, end);
                if (capture.name.empty())
                {
                    throw syntax_compilation_error("Expected capture name");
                }
                capture.location = ctx.get_location_optional(capture_begin, pos);
                captures.push_back(std::move(capture));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    return captures;
                }
                if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']'");
                }
                skip_whitespace_and_comments(pos, end);
            }
        }

        inline expression_lambda parse_lambda_expression(parsing_context& ctx)
        {
            auto& pos = ctx.iter_pos;
            auto end = ctx.iter_end;
            auto begin = pos;
            if (!skip_symbol_if_is(pos, end, "-<"))
            {
                throw syntax_compilation_error("Expected '-<'");
            }

            expression_lambda lambda;
            skip_whitespace_and_comments(pos, end);
            if (peek_symbol(pos, end).starts_with("["))
            {
                lambda.has_explicit_capture_list = true;
                lambda.captures = parse_lambda_captures(ctx);
                skip_whitespace_and_comments(pos, end);
            }

            if (peek_symbol(pos, end).starts_with("("))
            {
                lambda.parameters = parse_function_parameters(ctx);
                skip_whitespace_and_comments(pos, end);
            }

            if (skip_symbol_if_is(pos, end, ":"))
            {
                skip_whitespace_and_comments(pos, end);
                lambda.return_type = parse_type_symbol(ctx);
                skip_whitespace_and_comments(pos, end);
            }

            if (auto block = try_parse_function_block(ctx); block.has_value())
            {
                lambda.body = std::move(*block);
                lambda.location = ctx.get_location_optional(begin, pos);
                return lambda;
            }

            if (!skip_symbol_if_is(pos, end, "="))
            {
                throw syntax_compilation_error("Expected lambda block body or '=' before lambda expression body");
            }
            skip_whitespace_and_comments(pos, end);

            auto expr_begin = pos;
            expression returned = parse_expression_impl(ctx);
            function_return_statement ret;
            ret.expr = std::move(returned);
            ret.location = ctx.get_location_optional(expr_begin, pos);
            function_block block;
            block.statements.push_back(std::move(ret));
            block.location = ctx.get_location_optional(expr_begin, pos);
            lambda.body = std::move(block);
            lambda.location = ctx.get_location_optional(begin, pos);
            return lambda;
        }

        template < typename It >
        bool binary_operator_v3(It& pos, It end, std::array< expression*, 10 >& operator_bindings, expression*& value_binding)
        {
            static const std::map< std::string, int > operators_map = [] {
                std::map< std::string, int > result = {
                // clang-format off

                // Base operators
                {":=", 0}, {":<", 0}, {"<->", 0}, // assignment, move, and swap

                // Logical operators
                {"&&", 1}, // and
                {"&!", 1}, // nand
                {"^^", 1}, // xor
                {"|!", 1}, // nor
                {"||", 1}, // or
                {"^>", 1}, // implies
                {"^<", 1}, // implied
                {"^!", 1}, // equilvalent/nxor

                // Comparison operators
                {"<=>", 2}, {"==", 2}, {"!=", 2}, {"<=", 2}, {">=", 2}, {"<", 2}, {">", 2},

                // Division and modulus
                {"/", 4}, {"%", 4},

                // Addition and subtraction
                {"+", 5}, {"-", 5}, // regular
                // TODO:
                //{"+~", 5}, {"-~", 5}, // wrap-around
                //{"+!", 5}, {"-!", 5}, // undefined overflow

                // Multiplication
                {"*", 6},

                // Exponenciation
                {"^", 7},

                // Bitwise operators (prefix with '#')
                {"#&&", 8}, // bitwise and
                {"#&!", 8}, // bitwise nand
                {"#^^", 8}, // bitwise xor
                {"#|!", 8}, // bitwise nor
                {"#||", 8}, // bitwise or
                {"#^>", 8}, // bitwise implies (A implies B)
                {"#^<", 8}, // bitwise implied (B implies A)
                {"#^!", 8}, // bitwise equivalent (nxor)
                {"#++", 8}, // bitwise shift up
                {"#--", 8}, // bitwise shift down
                {"#+%", 8}, // bitwise up-rotate
                {"#-%", 8}  // bitwise down-rotate
                // clang-format on
                };
                for (auto const& compound_assignment : compound_assignment_operators)
                {
                    result[compound_assignment.first] = 0;
                }
                return result;
            }();

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
            *binding_point2 = std::move(new_expression);
            expression* binding_pointer = &as< expression_binary >(*binding_point2).rhs;
            for (std::size_t i = static_cast< std::size_t >(priority) + 1; i < operator_bindings.size(); i++)
            {
                operator_bindings[i] = binding_pointer;
            }
            value_binding = &(as< expression_binary >(*binding_point2)).rhs;
            return true;
        }

        inline std::optional< expression > try_parse_expression_impl(parsing_context& ctx)
        {
            auto& pos = ctx.iter_pos;
            auto end = ctx.iter_end;
            static localizer loc;

            // TODO: Make this not hardcoded
            std::string lang = "EN";

        skip_whitespace_and_comments(pos, end);

        QUXLANG_DEBUG_NAMED_VALUE(remaining, std::string(pos, end));

        expression result;
        std::array< expression*, 10 > bindings{};

        for (auto& binding : bindings)
        {
            binding = &result;
        }

        expression* value_bind_point = &result;
        bool have_anything = false;

    next_value:

        QUXLANG_DEBUG(remaining = std::string(pos, end);)
        std::optional< type_symbol > sym;
        skip_whitespace_and_comments(pos, end);

        auto kw_pre_translate = next_keyword(pos, end);
        auto kw = loc.translate_from(lang, kw_pre_translate);

        if (kw.has_value() && loc.is_value_kw(*kw))
        {
            if (!skip_keyword_if_is(pos, end, kw_pre_translate))
            {
                throw syntax_compilation_error("Expected value keyword");
            }
            expression_value_keyword expr_kw;
            expr_kw.keyword = *kw;
            *value_bind_point = std::move(expr_kw);
            have_anything = true;
        }
        else if (peek_symbol(pos, end).starts_with("-<"))
        {
            *value_bind_point = parse_lambda_expression(ctx);
            have_anything = true;
        }
        else if (auto str = try_parse_string_literal(pos, end); str)
        {
            expression_string_literal str_lit;
            str_lit.value = std::move(*str);
            *value_bind_point = std::move(str_lit);
            have_anything = true;
        }
        else if (auto chr = try_parse_char_literal(pos, end); chr)
        {
            expression_char_literal chr_lit;
            chr_lit.value = chr.value();
            *value_bind_point = std::move(chr_lit);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "IS_SIGNED"))
        {
            expression_is_signed expr_is_integral;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after IS_SIGNED");
            }
            expr_is_integral.of_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after IS_SIGNED(<type>");
            }
            *value_bind_point = std::move(expr_is_integral);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "IS_INTEGRAL"))
        {
            expression_is_integral expr_is_integral;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after IS_INTEGRAL");
            }
            expr_is_integral.of_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after IS_INTEGRAL(<type>");
            }
            *value_bind_point = std::move(expr_is_integral);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "SAME_TYPES"))
        {
            expression_same_types expr_same_types;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after SAME_TYPES");
            }
            expr_same_types.lhs_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' after first SAME_TYPES(<type>, ...)");
            }
            expr_same_types.rhs_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after SAME_TYPES(<lhs>, <rhs>)");
            }
            *value_bind_point = std::move(expr_same_types);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "STATIC_CHOOSE"))
        {
            expression_static_choose expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after STATIC_CHOOSE");
            }
            expr.condition = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);

            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' after STATIC_CHOOSE condition");
            }
            expr.true_expr = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);

            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' after STATIC_CHOOSE true expression");
            }
            expr.false_expr = parse_expression_impl(ctx);

            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after STATIC_CHOOSE(<cond>, <true>, <false>");
            }
            *value_bind_point = std::move(expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "SNAPSHOT"))
        {
            expression_snapshot expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after SNAPSHOT");
            }
            skip_whitespace_and_comments(pos, end);
            expr.name = parse_identifier(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after SNAPSHOT(<name>)");
            }
            *value_bind_point = std::move(expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "PACK_SIZE"))
        {
            expression_pack_size expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after PACK_SIZE");
            }
            skip_whitespace_and_comments(pos, end);
            expr.pack_name = parse_identifier(pos, end);
            if (expr.pack_name.empty())
            {
                throw syntax_compilation_error("Expected pack name in PACK_SIZE(<pack>)");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after PACK_SIZE(<pack>)");
            }
            *value_bind_point = std::move(expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "PACK_ARG"))
        {
            expression_pack_arg expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after PACK_ARG");
            }
            skip_whitespace_and_comments(pos, end);
            expr.pack_name = parse_identifier(pos, end);
            if (expr.pack_name.empty())
            {
                throw syntax_compilation_error("Expected pack name in PACK_ARG(<pack>, <index>)");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw syntax_compilation_error("Expected ',' after PACK_ARG pack name");
            }
            expr.index = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after PACK_ARG(<pack>, <index>)");
            }
            *value_bind_point = std::move(expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "FORWARD"))
        {
            expression_forward expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after FORWARD");
            }
            skip_whitespace_and_comments(pos, end);
            expr.symbol = parse_type_symbol(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after FORWARD symbol");
            }
            *value_bind_point = std::move(expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "SIZEOF"))
        {
            expression_sizeof sz;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after SIZEOF");
            }
            sz.of_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after SIZEOF(<type>");
            }
            *value_bind_point = std::move(sz);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "ALIGNOF"))
        {
            expression_alignof align;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after ALIGNOF");
            }
            align.of_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after ALIGNOF(<type>");
            }
            *value_bind_point = std::move(align);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "BITS"))
        {
            expression_bits bits;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after SIZEOF");
            }
            bits.of_type = try_parse_type_symbol(ctx).value();
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after SIZEOF(<type>");
            }
            *value_bind_point = std::move(bits);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "TARGET"))
        {
            expression_target tg;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after TARGET");
            }
            tg.target = std::move(try_parse_string_literal(pos, end).value());
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after TARGET(\"...\"");
            }
            *value_bind_point = std::move(tg);
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
        }
        else if (skip_keyword_if_is(pos, end, "PUN"))
        {
            expression_pun pun_expr;
            skip_whitespace_and_comments(pos, end);
            auto parsed_value = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "AS"))
            {
                skip_whitespace_and_comments(pos, end);
                auto as_type = try_parse_type_symbol(ctx);
                if (!as_type)
                {
                    throw syntax_compilation_error("Expected type after PUN ... AS");
                }
                pun_expr.value = std::move(parsed_value);
                pun_expr.as_type = std::move(*as_type);
            }
            else if (parsed_value.template type_is< expression_typecast >() && !parsed_value.template get_as< expression_typecast >().keyword.has_value())
            {
                auto& cast_expr = parsed_value.template get_as< expression_typecast >();
                pun_expr.value = std::move(cast_expr.expr);
                pun_expr.as_type = std::move(cast_expr.to_type);
            }
            else
            {
                throw syntax_compilation_error("Expected 'AS <type>' after PUN expression");
            }
            *value_bind_point = std::move(pun_expr);
            have_anything = true;
        }
        else if (parse_iterator const unwrap_begin = pos; skip_keyword_if_is(pos, end, "UNWRAP"))
        {
            expression_variant_unwrap unwrap_expr;
            skip_whitespace_and_comments(pos, end);
            unwrap_expr.subject = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "INTO"))
            {
                throw syntax_compilation_error("Expected INTO after UNWRAP expression");
            }
            skip_whitespace_and_comments(pos, end);
            std::optional< type_symbol > const target_type = try_parse_type_symbol(ctx);
            if (!target_type.has_value())
            {
                throw syntax_compilation_error("Expected type after UNWRAP expression INTO");
            }
            unwrap_expr.type = *target_type;
            unwrap_expr.location = ctx.get_location_optional(unwrap_begin, pos);
            *value_bind_point = std::move(unwrap_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "PLACE"))
        {
            expression_place place_expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "AT"))
            {
                throw syntax_compilation_error("Expected 'AT' after 'PLACE'");
            }
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw syntax_compilation_error("Expected '(' after PLACE AT");
            }
            skip_whitespace_and_comments(pos, end);
            place_expr.at = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')' after PLACE AT(location expression)");
            }
            skip_whitespace_and_comments(pos, end);
            place_expr.type = parse_type_symbol(ctx);
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ":("))
            {
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    while (true)
                    {
                        skip_whitespace_and_comments(pos, end);
                        place_expr.args.push_back(parse_expression_arg(ctx));
                        skip_whitespace_and_comments(pos, end);
                        if (skip_symbol_if_is(pos, end, ","))
                        {
                            continue;
                        }
                        if (skip_symbol_if_is(pos, end, ")"))
                        {
                            break;
                        }
                        throw syntax_compilation_error("Expected ',' or ')' in PLACE args");
                    }
                }
            }
            else if (skip_symbol_if_is(pos, end, ":="))
            {
                skip_whitespace_and_comments(pos, end);
                place_expr.assign_init = parse_expression_impl(ctx);
            }
            *value_bind_point = std::move(place_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "ADDRESS_LAUNDER_FROM"))
        {
            expression_address_launder_from launder_expr;
            skip_whitespace_and_comments(pos, end);
            launder_expr.pointer = parse_expression_impl(ctx);
            *value_bind_point = std::move(launder_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "ADDRESS_LAUNDER"))
        {
            expression_address_launder launder_expr;
            skip_whitespace_and_comments(pos, end);
            launder_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "TO"))
            {
                throw syntax_compilation_error("Expected 'TO' after ADDRESS_LAUNDER <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            auto to_type = try_parse_type_symbol(ctx);
            if (!to_type)
            {
                throw syntax_compilation_error("Expected type after ADDRESS_LAUNDER <expr> TO");
            }
            launder_expr.to_type = std::move(*to_type);
            *value_bind_point = std::move(launder_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "BEGIN_ALLOC_REGION"))
        {
            expression_begin_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "TO"))
            {
                throw syntax_compilation_error("Expected 'TO' after BEGIN_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            auto as_type = try_parse_type_symbol(ctx);
            if (!as_type)
            {
                throw syntax_compilation_error("Expected type after BEGIN_ALLOC_REGION <expr> TO");
            }
            region_expr.as_type = std::move(*as_type);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "END_ALLOC_REGION"))
        {
            expression_end_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.pointer = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "BEGIN_MULTI_ALLOC_REGION"))
        {
            expression_begin_multi_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "SIZE"))
            {
                throw syntax_compilation_error("Expected 'SIZE' after BEGIN_MULTI_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.count = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "TO"))
            {
                throw syntax_compilation_error("Expected 'TO' after BEGIN_MULTI_ALLOC_REGION <expr> SIZE <count>");
            }
            skip_whitespace_and_comments(pos, end);
            auto as_type = try_parse_type_symbol(ctx);
            if (!as_type)
            {
                throw syntax_compilation_error("Expected type after BEGIN_MULTI_ALLOC_REGION ... TO");
            }
            region_expr.as_type = std::move(*as_type);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "END_MULTI_ALLOC_REGION"))
        {
            expression_end_multi_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.pointer = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (skip_keyword_if_is(pos, end, "SIZE"))
            {
                skip_whitespace_and_comments(pos, end);
                region_expr.count = parse_expression_impl(ctx);
            }
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "RESIZE_MULTI_ALLOC_REGION"))
        {
            expression_resize_multi_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.pointer = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "COUNT"))
            {
                throw syntax_compilation_error("Expected 'COUNT' after RESIZE_MULTI_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.newcount = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "BEGIN_DYNAMIC_ALLOC_REGION"))
        {
            expression_begin_dynamic_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "SIZE"))
            {
                throw syntax_compilation_error("Expected 'SIZE' after BEGIN_DYNAMIC_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.count = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "END_DYNAMIC_ALLOC_REGION"))
        {
            expression_end_dynamic_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "SIZE"))
            {
                throw syntax_compilation_error("Expected 'SIZE' after END_DYNAMIC_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.count = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "RESIZE_DYNAMIC_ALLOC_REGION"))
        {
            expression_resize_dynamic_alloc_region region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.address = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "SIZE"))
            {
                throw syntax_compilation_error("Expected 'SIZE' after RESIZE_DYNAMIC_ALLOC_REGION <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.newsize = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "PARENT_ALLOC_ADDRESS"))
        {
            expression_parent_alloc_address region_expr;
            skip_whitespace_and_comments(pos, end);
            region_expr.pointer_or_address = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (skip_keyword_if_is(pos, end, "RELOCATE_REGION_OBJECTS"))
        {
            expression_relocate_region_objects region_expr;
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "FROM"))
            {
                throw syntax_compilation_error("Expected 'FROM' after RELOCATE_REGION_OBJECTS");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.from = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "TO"))
            {
                throw syntax_compilation_error("Expected 'TO' after RELOCATE_REGION_OBJECTS FROM <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.to = parse_expression_impl(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_keyword_if_is(pos, end, "SIZE"))
            {
                throw syntax_compilation_error("Expected 'SIZE' after RELOCATE_REGION_OBJECTS FROM <expr> TO <expr>");
            }
            skip_whitespace_and_comments(pos, end);
            region_expr.byte_count = parse_expression_impl(ctx);
            *value_bind_point = std::move(region_expr);
            have_anything = true;
        }
        else if (auto number_end = iter_parse_number(pos, end); number_end != pos)
        {
            expression_numeric_literal num;
            num.value = std::string(pos, number_end);
            pos = number_end;
            *value_bind_point = std::move(num);
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            skip_whitespace_and_comments(pos, end);
            expression_thisdot_reference thisdot;
            thisdot.field_name = parse_subentity(pos, end);
            *value_bind_point = std::move(thisdot);
            have_anything = true;
        }
        else if (auto sym = try_parse_type_symbol(ctx); sym)
        {
            assert(sym.has_value());

            expression_symbol_reference lvalue;
            lvalue.symbol = std::move(*sym);

            *value_bind_point = std::move(lvalue);
            have_anything = true;
        }
        else if (skip_symbol_if_is(pos, end, "("))
        {

            expression parenthesis;
            parenthesis = parse_expression_impl(ctx);
            *value_bind_point = std::move(parenthesis);
            have_anything = true;
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw syntax_compilation_error("Expected ')'");
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
                throw syntax_compilation_error("Expected binary operator to be followed by value");
            }
        }

    next_operator:
        QUXLANG_DEBUG(remaining = std::string(pos, end);)

        skip_whitespace_and_comments(pos, end);
        if (detail::binary_operator_v3(pos, end, bindings, value_bind_point))
        {
            goto next_value;
        }
        else if (std::optional< expression_call > call = try_parse_function_callsite_expression(ctx); call)
        {
            expression* binding_point2 = bindings[bindings.size() - 1];
            auto call_expr = std::move(*call);
            call_expr.callee = std::move(*binding_point2);
            *binding_point2 = quxlang::expression(std::move(call_expr));
            goto next_operator;
        }
        else if (parse_iterator const isa_begin = pos; skip_keyword_if_is(pos, end, "ISA"))
        {
            skip_whitespace_and_comments(pos, end);
            std::optional< type_symbol > const target_type = try_parse_type_symbol(ctx);
            if (!target_type.has_value())
            {
                throw syntax_compilation_error("Expected type after ISA");
            }
            expression_variant_isa isa;
            isa.subject = std::move(*bindings[2]);
            isa.type = *target_type;
            isa.location = ctx.get_location_optional(isa_begin, pos);
            *bindings[2] = std::move(isa);
            for (std::size_t i = 3; i < bindings.size(); ++i)
            {
                bindings[i] = bindings[2];
            }
            goto next_operator;
        }
        else if (parse_iterator const is_begin = pos; skip_keyword_if_is(pos, end, "IS"))
        {
            skip_whitespace_and_comments(pos, end);
            std::string option_name = parse_subentity(pos, end);
            if (option_name.empty())
            {
                throw syntax_compilation_error("Expected UNION option name after IS");
            }
            expression_union_is is;
            is.subject = std::move(*bindings[2]);
            is.option_name = std::move(option_name);
            is.location = ctx.get_location_optional(is_begin, pos);
            *bindings[2] = std::move(is);
            for (std::size_t i = 3; i < bindings.size(); ++i)
            {
                bindings[i] = bindings[2];
            }
            goto next_operator;
        }
        else if (skip_keyword_if_is(pos, end, "AS"))
        {
            // Type cast operator with precedence level 3
            // Bind like a binary operator at priority 3, but the RHS is a type symbol
            expression_typecast tc;

            skip_whitespace_and_comments(pos, end);
            if (auto kw = skip_keyword_if_one_of(pos, end, {"EXPLICIT", "REINTERPRET", "PARTIAL", "ASSUME", "CHECKED", "APPROXIMATE"}); kw)
            {
                tc.keyword = *kw;
                skip_whitespace_and_comments(pos, end);
            }

            // Parse the destination type
            auto to_type = try_parse_type_symbol(ctx);
            if (!to_type)
            {
                throw syntax_compilation_error("Expected type after AS (optional EXPLICIT/REINTERPRET/PARTIAL/ASSUME/CHECKED/APPROXIMATE)");
            }
            tc.to_type = std::move(*to_type);

            // Insert the node at precedence level 3
            expression* binding_point2 = bindings[3];
            tc.expr = std::move(*binding_point2);
            *binding_point2 = quxlang::expression(std::move(tc));

            // For higher-precedence operators, they should bind to the result of the cast
            for (int i = 4; i < bindings.size(); ++i)
            {
                bindings[i] = binding_point2;
            }

            // Continue parsing more postfix/operators
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            expression_dotreference dot;
            dot.field_name = parse_subentity(pos, end);
            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "#("))
            {
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    while (true)
                    {
                        skip_whitespace_and_comments(pos, end);
                        dot.template_arguments.push_back(parse_initialization_expression_arg(ctx));
                        skip_whitespace_and_comments(pos, end);
                        if (skip_symbol_if_is(pos, end, ")"))
                        {
                            break;
                        }
                        if (!skip_symbol_if_is(pos, end, ","))
                        {
                            throw syntax_compilation_error("expected ',' or ')'");
                        }
                    }
                }
            }
            else if (skip_symbol_if_is(pos, end, "#"))
            {
                parse_iterator const arg_begin = pos;
                auto arg_symbol = try_parse_type_symbol(ctx);
                if (!arg_symbol.has_value())
                {
                    throw syntax_compilation_error("expected symbol after '#'");
                }

                expression_symbol_reference symbol_arg;
                symbol_arg.symbol = std::move(*arg_symbol);

                expression_arg arg;
                arg.name = "T";
                arg.value = std::move(symbol_arg);
                arg.location = ctx.get_location_optional(arg_begin, pos);
                dot.template_arguments.push_back(std::move(arg));
            }
            dot.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(dot);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, ":("))
        {
            // TODO: This part
            throw rpnx::unimplemented();
        }
        else if (skip_symbol_if_is(pos, end, "->"))
        {
            // expression_rightarrow arrow;
            expression_unary_postfix arrow;
            arrow.operator_str = "->";
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
            expression_multibind brkts;
            brkts.operator_str = "OPERATOR[]";
            brkts.lhs = std::move(*bindings[bindings.size() - 1]);
            while (true)
            {
                brkts.bracketed.push_back(parse_expression_impl(ctx));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                else if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']' in brackets");
                }
            }
            *bindings[bindings.size() - 1] = std::move(brkts);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "[&"))
        {
            expression_multibind brkts;
            brkts.operator_str = "OPERATOR[&]";
            brkts.lhs = std::move(*bindings[bindings.size() - 1]);
            while (true)
            {
                brkts.bracketed.push_back(parse_expression_impl(ctx));
                skip_whitespace_and_comments(pos, end);
                if (skip_symbol_if_is(pos, end, "]"))
                {
                    break;
                }
                else if (!skip_symbol_if_is(pos, end, ","))
                {
                    throw syntax_compilation_error("Expected ',' or ']' in brackets");
                }
            }
            *bindings[bindings.size() - 1] = std::move(brkts);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "??"))
        {
            expression_unary_postfix blt;
            blt.operator_str = "??";
            blt.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(blt);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "?!"))
        {
            expression_unary_postfix blt;
            blt.operator_str = "?!";
            blt.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(blt);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "++"))
        {
            expression_unary_postfix inc;
            inc.operator_str = "++";
            inc.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(inc);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "--"))
        {
            expression_unary_postfix dec;
            dec.operator_str = "--";
            dec.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(dec);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "#!!"))
        {
            expression_unary_postfix bnt;
            bnt.operator_str = "#!!";
            bnt.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(bnt);
            goto next_operator;
        }
        else if (skip_symbol_if_is(pos, end, "!!"))
        {
            expression_unary_postfix bnt;
            bnt.operator_str = "!!";
            bnt.lhs = std::move(*bindings[bindings.size() - 1]);
            *bindings[bindings.size() - 1] = std::move(bnt);
            goto next_operator;
        }
        else
        {
            return std::move(result);
        }

        throw rpnx::unimplemented();
    }
    } // namespace detail

    inline std::optional< expression > try_parse_expression(parsing_context& ctx)
    {
        skip_whitespace_and_comments(ctx.iter_pos, ctx.iter_end);
        auto begin = ctx.iter_pos;
        auto result = detail::try_parse_expression_impl(ctx);
        if (result)
        {
            set_location(*result, ctx.get_location_optional(begin, ctx.iter_pos));
        }
        return result;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_EXPRESSION_HPP
