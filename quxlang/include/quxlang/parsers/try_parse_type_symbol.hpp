// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_TYPE_SYMBOL_HEADER_GUARD

#include <optional>
#include <utility>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/parsers/context.hpp>
#include <quxlang/parsers/function.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_subentity.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_integral_keyword.hpp>

namespace quxlang::parsers
{
    expression parse_expression(parsing_context& ctx);
    type_symbol parse_type_symbol(parsing_context& ctx);
    argif parse_argif(parsing_context& ctx);

    inline auto parse_initialization_expression_arg(parsing_context& ctx) -> expression_arg
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = pos;
        expression_arg result;

        if (skip_symbol_if_is(pos, end, "@"))
        {
            result.name = parse_argument_name(pos, end);
            if (result.name->empty())
            {
                throw std::logic_error("Expected identifier after '@' in instanciation argument");
            }
            skip_whitespace_and_comments(pos, end);
        }

        result.value = parse_expression(ctx);
        result.location = ctx.get_location_optional(begin, pos);
        return result;
    }

    inline std::optional< type_symbol > try_parse_type_symbol(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        type_symbol output = context_reference{};
        QUXLANG_DEBUG_NAMED_VALUE(remaining, std::string(pos, end));
        skip_whitespace_and_comments(pos, end);
    start:
        skip_whitespace(pos, end);
        if (skip_keyword_if_is(pos, end, "MODULE"))
        {
            absolute_module_reference m;
            // TODO: Only allow this to be parsed from unit tests etc.
            // or add imported_module_reference or something.
            skip_whitespace(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after MODULE");
            }
            m.module_name = parse_identifier(pos, end);
            skip_whitespace(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::logic_error("Expected ')' after MODULE(" + m.module_name + ")");
            }
            output = std::move(m);
        }
        else if (skip_keyword_if_is(pos, end, "NUMERIC_LITERAL"))
        {
            output = numeric_literal_reference{};
        }
        else if (skip_keyword_if_is(pos, end, "STRING_CONSTANT"))
        {
            output = readonly_constant{.kind = constant_kind::string};
        }
        else if (skip_keyword_if_is(pos, end, "CSTRING_CONSTANT"))
        {
            output = readonly_constant{.kind = constant_kind::cstring};
        }
        else if (skip_keyword_if_is(pos, end, "DATA_CONSTANT"))
        {
            output = readonly_constant{.kind = constant_kind::data};
        }
        else if (skip_keyword_if_is(pos, end, "NUMERIC_CONSTANT"))
        {
            output = readonly_constant{.kind = constant_kind::numeric};
        }
        else if (skip_keyword_if_is(pos, end, "VOID"))
        {
            output = void_type{};
        }
        else if (skip_keyword_if_is(pos, end, "BOOL"))
        {
            output = bool_type{};
        }
        else if (skip_keyword_if_is(pos, end, "BYTE"))
        {
            output = byte_type{};
        }
        else if (skip_keyword_if_is(pos, end, "__CONSTEXPR_PROXY"))
        {
            throw std::logic_error("__CONSTEXPR_PROXY is an internal implementation detail and cannot be named in source");
        }
        else if (skip_keyword_if_is(pos, end, "PROCEDURE"))
        {
            procedure_type result;
            result.signature.return_type = void_type{};

            skip_whitespace_and_comments(pos, end);
            if (auto calling_convention = skip_keyword_if_one_of(pos, end, {"CCALL", "STDCALL"}))
            {
                result.calling_convention = *calling_convention;
            }

            skip_whitespace_and_comments(pos, end);
            result.is_noexcept = skip_keyword_if_is(pos, end, "NOEXCEPT");

            skip_whitespace_and_comments(pos, end);
            auto unexpected_modifier = next_keyword(pos, end);
            if (!unexpected_modifier.empty())
            {
                throw std::logic_error("Unexpected PROCEDURE modifier keyword: " + unexpected_modifier);
            }

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after PROCEDURE");
            }

            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = std::move(result);
            }
            else
            {
                while (true)
                {
                    skip_whitespace_and_comments(pos, end);

                    if (skip_symbol_if_is(pos, end, ":"))
                    {
                        skip_whitespace_and_comments(pos, end);
                        result.signature.return_type = parse_type_symbol(ctx);
                        skip_whitespace_and_comments(pos, end);
                        if (!skip_symbol_if_is(pos, end, ")"))
                        {
                            throw std::logic_error("Expected ')' after PROCEDURE return type");
                        }
                        break;
                    }

                    if (skip_symbol_if_is(pos, end, "@"))
                    {
                        auto arg_name = parse_argument_name(pos, end);
                        skip_whitespace_and_comments(pos, end);
                        result.signature.params.named[arg_name] = parse_type_symbol(ctx);
                    }
                    else
                    {
                        result.signature.params.positional.push_back(parse_type_symbol(ctx));
                    }

                    skip_whitespace_and_comments(pos, end);
                    if (skip_symbol_if_is(pos, end, ","))
                    {
                        continue;
                    }
                    if (skip_symbol_if_is(pos, end, ":"))
                    {
                        skip_whitespace_and_comments(pos, end);
                        result.signature.return_type = parse_type_symbol(ctx);
                        skip_whitespace_and_comments(pos, end);
                        if (!skip_symbol_if_is(pos, end, ")"))
                        {
                            throw std::logic_error("Expected ')' after PROCEDURE return type");
                        }
                        break;
                    }
                    if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }
                    throw std::logic_error("Expected ',', ':', or ')' in PROCEDURE type");
                }

                output = std::move(result);
            }
        }
        else if (skip_keyword_if_is(pos, end, "INITGUARD"))
        {
            output = initguard_type{};
        }
        else if (skip_keyword_if_is(pos, end, "INITGUARD_LOCK"))
        {
            output = initguard_lock_type{};
        }
        else if (skip_keyword_if_is(pos, end, "STORAGE"))
        {
            storage result;

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after STORAGE");
            }

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                while (true)
                {
                    result.storable_types.insert(parse_type_symbol(ctx));
                    skip_whitespace_and_comments(pos, end);
                    if (skip_symbol_if_is(pos, end, ","))
                    {
                        skip_whitespace_and_comments(pos, end);
                        continue;
                    }
                    if (skip_symbol_if_is(pos, end, ")"))
                    {
                        break;
                    }
                    throw std::logic_error("Expected ',' or ')' after STORAGE type list");
                }
            }

            if (result.storable_types.empty())
            {
                throw std::logic_error("STORAGE requires at least one type");
            }

            output = std::move(result);
        }
        else if (skip_keyword_if_is(pos, end, "ALIGNED_STORAGE"))
        {
            aligned_storage result;

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after ALIGNED_STORAGE");
            }

            skip_whitespace_and_comments(pos, end);
            result.size = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("Expected ',' after ALIGNED_STORAGE size");
            }
            skip_whitespace_and_comments(pos, end);
            result.align = parse_expression(ctx);
            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::logic_error("Expected ')' after ALIGNED_STORAGE(size, align)");
            }

            output = std::move(result);
        }
        else if (skip_keyword_if_is(pos, end, "PACK_ARG_TYPE"))
        {
            pack_arg_type_ref result;

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, "("))
            {
                throw std::logic_error("Expected '(' after PACK_ARG_TYPE");
            }

            skip_whitespace_and_comments(pos, end);
            result.pack_name = parse_identifier(pos, end);
            if (result.pack_name.empty())
            {
                throw std::logic_error("Expected pack name in PACK_ARG_TYPE");
            }

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("Expected ',' after PACK_ARG_TYPE pack name");
            }

            skip_whitespace_and_comments(pos, end);
            result.index = parse_expression(ctx);

            skip_whitespace_and_comments(pos, end);
            if (!skip_symbol_if_is(pos, end, ")"))
            {
                throw std::logic_error("Expected ')' after PACK_ARG_TYPE index");
            }

            output = std::move(result);
        }
        else if (auto float_kw = try_parse_float_keyword(pos, end); float_kw)
        {
            output = std::move(*float_kw);
        }
        else if (auto int_kw = try_parse_integral_keyword(pos, end); int_kw)
        {
            output = std::move(*int_kw);
        }
        else if (skip_symbol_if_is(pos, end, "["))
        {
            array_type arr;

            skip_whitespace(pos, end);
            arr.element_count = parse_expression(ctx);

            skip_whitespace(pos, end);
            if (!skip_symbol_if_is(pos, end, "]"))
            {
                throw std::logic_error("Expected ']' after array count");
            }

            skip_whitespace(pos, end);

            arr.element_type = parse_type_symbol(ctx);

            output = std::move(arr);
        }
        else if (skip_keyword_if_is(pos, end, "AUTO"))
        {
            auto_temploidic tref;

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "&"))
            {
                return ptrref_type{.target = parse_type_symbol(ctx), .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
            }

            if (skip_symbol_if_is(pos, end, "("))
            {
                skip_whitespace_and_comments(pos, end);
                tref.name = parse_identifier(pos, end);
                if (tref.name.empty())
                {
                    throw std::logic_error("Expected identifier after T(");
                }
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw std::logic_error("Expected ')' after T(" + tref.name);
                }
            }

            output = std::move(tref);
        }
        else if (skip_keyword_if_is(pos, end, "TT"))
        {
            type_temploidic tref;

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "("))
            {
                skip_whitespace_and_comments(pos, end);
                tref.name = parse_identifier(pos, end);
                if (tref.name.empty())
                {
                    throw std::logic_error("Expected identifier after T(");
                }
                skip_whitespace_and_comments(pos, end);
                if (!skip_symbol_if_is(pos, end, ")"))
                {
                    throw std::logic_error("Expected ')' after T(" + tref.name);
                }
            }

            output = std::move(tref);
        }
        else if (skip_symbol_if_is(pos, end, "&"))
        {
            return ptrref_type{.target = parse_type_symbol(ctx), .ptr_class = pointer_class::ref, .qual = qualifier::mut};
        }
        else if (auto kw = skip_keyword_if_one_of(pos, end, {"MUT", "CONST", "WRITE", "TEMP"}); kw != std::nullopt)
        {
            ptrref_type pr_result;
            if (*kw == "MUT")
            {
                pr_result.qual = qualifier::mut;
            }
            else if (*kw == "CONST")
            {
                pr_result.qual = qualifier::constant;
            }
            else if (*kw == "WRITE")
            {
                pr_result.qual = qualifier::write;
            }
            else if (*kw == "TEMP")
            {
                pr_result.qual = qualifier::temp;
            }
            else
            {
                throw std::logic_error("unreachable");
            }
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "&"))
            {
                pr_result.ptr_class = pointer_class::ref;
            }
            else if (skip_symbol_if_is(pos, end, "->"))
            {
                pr_result.ptr_class = pointer_class::instance;
            }
            else if (skip_symbol_if_is(pos, end, "=>>"))
            {
                pr_result.ptr_class = pointer_class::array;
            }
            else
            {
                throw std::logic_error(std::string("Expected &, ->, or =>> after ") + std::string(*kw));
            }
            pr_result.target = parse_type_symbol(ctx);
            return pr_result;
        }
        else if (skip_keyword_if_is(pos, end, "NEW"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after NEW");
            }
            return nvalue_slot{parse_type_symbol(ctx)};
        }
        else if (skip_keyword_if_is(pos, end, "DESTROY"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected && after DESTROY");
            }
            return dvalue_slot{parse_type_symbol(ctx)};
        }
        else if (skip_keyword_if_is(pos, end, "SZ"))
        {
            return size_type{};
        }
        else if (auto paren_pos = pos; skip_symbol_if_is(pos, end, "("))
        {
            auto grouped_ctx = ctx;
            grouped_ctx.iter_pos = pos;
            auto grouped = try_parse_type_symbol(grouped_ctx);
            skip_whitespace_and_comments(grouped_ctx.iter_pos, grouped_ctx.iter_end);
            if (!grouped || !skip_symbol_if_is(grouped_ctx.iter_pos, grouped_ctx.iter_end, ")"))
            {
                pos = paren_pos;
                return std::nullopt;
            }
            output = std::move(*grouped);
            pos = grouped_ctx.iter_pos;
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
                throw std::logic_error("expected identifier after ::");

            output = subsymbol{context_reference(), std::move(ident)};
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = submember{freebound_identifier{"THIS"}, std::move(ident)};
        }
        else if (skip_symbol_if_is(pos, end, "->"))
        {
            return ptrref_type{.target = parse_type_symbol(ctx), .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        }
        else if (skip_symbol_if_is(pos, end, "=>>"))
        {
            return ptrref_type{.target = parse_type_symbol(ctx), .ptr_class = pointer_class::array, .qual = qualifier::mut};
        }
        else
        {
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = freebound_identifier{.name = std::move(ident)};
        }

    check_next:
        skip_whitespace_and_comments(pos, end);

        QUXLANG_DEBUG(
            if (remaining.starts_with("I32::") || remaining.starts_with("::CON") || remaining.starts_with("CON"))
            {
                int x = 0;
            }
        )

        QUXLANG_DEBUG(remaining = std::string(pos, end);)

        if (skip_symbol_if_is(pos, end, "::."))
        {
            auto ident = parse_subentity(pos, end);
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            if (ident.empty())
            {
                return output;
            }

            output = submember{std::move(output), std::move(ident)};
            goto check_next;
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return output;
            }

            output = subsymbol{std::move(output), std::move(ident)};
            goto check_next;
        }
        else if (skip_symbol_if_is(pos, end, "#("))
        {
            initialization_reference param_set;
            param_set.initializee = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = std::move(param_set);
                goto check_next;
            }
        next_arg:
            skip_whitespace_and_comments(pos, end);

            param_set.arguments.push_back(parse_initialization_expression_arg(ctx));

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = std::move(param_set);
                goto check_next;
            }
            else if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("expected ',' or ')'");
            }
            goto next_arg;
        }
        else if (skip_symbol_if_is(pos, end, "#"))
        {
            initialization_reference param_set;
            param_set.initializee = std::move(output);

            auto const arg_begin = pos;
            auto arg_symbol = try_parse_type_symbol(ctx);
            if (!arg_symbol.has_value())
            {
                throw std::logic_error("expected symbol after '#'");
            }

            expression_symbol_reference symbol_arg;
            symbol_arg.symbol = std::move(*arg_symbol);

            expression_arg arg;
            arg.name = "T";
            arg.value = std::move(symbol_arg);
            arg.location = ctx.get_location_optional(arg_begin, pos);
            param_set.arguments.push_back(std::move(arg));

            output = std::move(param_set);
            goto check_next;
        }
        else if (skip_symbol_if_is(pos, end, "#{"))
        {
            QUXLANG_DEBUG(remaining = std::string(pos, end);)

            instanciation_reference param_set;

            param_set.temploid.templexoid = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                output = std::move(param_set);
                goto check_next;
            }

            skip_whitespace_and_comments(pos, end);

        next_arg2:
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                argif seltype = parse_argif(ctx);
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    skip_whitespace(pos, end);
                    param_set.params.named[param_name] = make_type_instantiation(parse_type_symbol(ctx));
                }
                else
                {
                    param_set.params.named[param_name] = make_type_instantiation(seltype.type);
                }
                param_set.temploid.which.interface.named[std::move(param_name)] = std::move(seltype);
            }
            else
            {
                argif seltype = parse_argif(ctx);
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    skip_whitespace(pos, end);
                    param_set.params.positional.push_back(make_type_instantiation(parse_type_symbol(ctx)));
                }
                else
                {
                    param_set.params.positional.push_back(make_type_instantiation(seltype.type));
                }
                param_set.temploid.which.interface.positional.push_back(std::move(seltype));
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                output = std::move(param_set);
                goto check_next;
            }
            else if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("expected ',' or '}'");
            }
            goto next_arg2;
        }
        else if (skip_symbol_if_is(pos, end, "#["))
        {
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            temploid_reference param_set;
            param_set.templexoid = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "]"))
            {
                output = std::move(param_set);
                goto check_next;
            }

            if (skip_keyword_if_is(pos, end, "BUILTIN"))
            {
                // TODO: consider if this is useful
                // param_set.which.builtin = true;

                skip_whitespace(pos, end);

                if (!skip_symbol_if_is(pos, end, ";"))
                {
                    throw std::logic_error("Expected ';'");
                }
            }

            skip_whitespace_and_comments(pos, end);
        next_arg3:
            QUXLANG_DEBUG(remaining = std::string(pos, end);)
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                param_set.which.interface.named[std::move(param_name)] = parse_argif(ctx);
            }
            else
            {
                param_set.which.interface.positional.push_back(parse_argif(ctx));
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "]"))
            {
                output = std::move(param_set);
                goto check_next;
            }
            else if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("expected ',' or ']'");
            }
            goto next_arg3;
        }

        return output;
    }

} // namespace quxlang::parsers

#include <quxlang/parsers/parse_expression.hpp>

#endif // TRY_PARSE_TYPE_SYMBOL_HPP
