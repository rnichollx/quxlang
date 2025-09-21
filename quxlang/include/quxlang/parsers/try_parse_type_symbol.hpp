// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_TYPE_SYMBOL_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_TYPE_SYMBOL_HEADER_GUARD

#include <optional>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/parsers/function.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/parse_identifier.hpp>
#include <quxlang/parsers/parse_subentity.hpp>
#include <quxlang/parsers/parse_whitespace_and_comments.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_integral_keyword.hpp>

namespace quxlang::parsers
{

    template < typename It >
    expression parse_expression(It& pos, It end);

    template < typename It >
    type_symbol parse_type_symbol(It& pos, It end);

    template < typename It >
    argif parse_argif(It& pos, It end);

    template < typename It >
    std::optional< type_symbol > try_parse_type_symbol(It& pos, It end)
    {
        type_symbol output = context_reference{};
        std::string remaining = std::string(pos, end);
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
            output = m;
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
        else if (auto int_kw = try_parse_integral_keyword(pos, end); int_kw)
        {
            output = *int_kw;
        }
        else if (skip_symbol_if_is(pos, end, "["))
        {
            array_type arr;

            skip_whitespace(pos, end);
            arr.element_count = parse_expression(pos, end);

            skip_whitespace(pos, end);
            if (!skip_symbol_if_is(pos, end, "]"))
            {
                throw std::logic_error("Expected ']' after array count");
            }

            skip_whitespace(pos, end);

            arr.element_type = parse_type_symbol(pos, end);

            output = arr;
        }
        else if (skip_keyword_if_is(pos, end, "AUTO"))
        {
            auto_temploidic tref;

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

            output = tref;
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

            output = tref;
        }
        else if (skip_symbol_if_is(pos, end, "&"))
        {
            return ptrref_type{.target = parse_type_symbol(pos, end), .ptr_class = pointer_class::ref, .qual = qualifier::mut};
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
                throw std::logic_error("Expected &, ->, or =>> after " + *kw);
            }
            pr_result.target = parse_type_symbol(pos, end);
            return pr_result;
        }
        else if (skip_keyword_if_is(pos, end, "NEW"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after NEW");
            }
            return nvalue_slot{parse_type_symbol(pos, end)};
        }
        else if (skip_keyword_if_is(pos, end, "DESTROY"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected && after DESTROY");
            }
            return dvalue_slot{parse_type_symbol(pos, end)};
        }
        else if (skip_keyword_if_is(pos, end, "AUTO"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after DESTROY");
            }
            return ptrref_type{.target = parse_type_symbol(pos, end), .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
        }
        else if (skip_keyword_if_is(pos, end, "SZ"))
        {
            return size_type{};
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
            std::string remaining = std::string(pos, end);
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = submember{freebound_identifier{"THIS"}, std::move(ident)};
        }
        else if (skip_symbol_if_is(pos, end, "->"))
        {
            return ptrref_type{.target = parse_type_symbol(pos, end), .ptr_class = pointer_class::instance, .qual = qualifier::mut};
        }
        else if (skip_symbol_if_is(pos, end, "=>>"))
        {
            return ptrref_type{.target = parse_type_symbol(pos, end), .ptr_class = pointer_class::array, .qual = qualifier::mut};
        }
        else
        {
            std::string remaining = std::string(pos, end);
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = freebound_identifier{.name = std::move(ident)};
        }

    check_next:
        skip_whitespace_and_comments(pos, end);

        if (remaining.starts_with("I32::") || remaining.starts_with("::CON") || remaining.starts_with("CON"))
        {
            int x = 0;
        }

        remaining = std::string(pos, end);

        if (skip_symbol_if_is(pos, end, "::"))
        {
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return output;
            }

            output = subsymbol{std::move(output), std::move(ident)};
            goto check_next;
        }
        else if (skip_symbol_if_is(pos, end, "::."))
        {
            auto ident = parse_subentity(pos, end);
            remaining = std::string(pos, end);
            if (ident.empty())
            {
                return output;
            }

            output = submember{std::move(output), std::move(ident)};
            goto check_next;
        }
        else if (skip_symbol_if_is(pos, end, "#("))
        {
            initialization_reference param_set;
            param_set.initializee = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = param_set;
                goto check_next;
            }
        next_arg:
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                skip_whitespace(pos, end);
                remaining = std::string(pos, end);
                param_set.parameters.named[param_name] = parse_type_symbol(pos, end);
            }
            else
            {
                param_set.parameters.positional.push_back(parse_type_symbol(pos, end));
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, ")"))
            {
                output = param_set;
                goto check_next;
            }
            else if (!skip_symbol_if_is(pos, end, ","))
            {
                throw std::logic_error("expected ',' or ')'");
            }
            goto next_arg;
        }
        else if (skip_symbol_if_is(pos, end, "#{"))
        {
            remaining = std::string(pos, end);

            instanciation_reference param_set;

            param_set.temploid.templexoid = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                output = param_set;
                goto check_next;
            }

            skip_whitespace_and_comments(pos, end);

        next_arg2:
            remaining = std::string(pos, end);
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                argif seltype = parse_argif(pos, end);
                param_set.temploid.which.interface.named[param_name] = seltype;
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    skip_whitespace(pos, end);
                    param_set.params.named[param_name] = parse_type_symbol(pos, end);
                }
                else
                {
                    param_set.params.named[param_name] = seltype.type;
                }
            }
            else
            {
                argif seltype = parse_argif(pos, end);
                param_set.temploid.which.interface.positional.push_back(seltype);
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    skip_whitespace(pos, end);
                    param_set.params.positional.push_back(parse_type_symbol(pos, end));
                }
                else
                {
                    param_set.params.positional.push_back(seltype.type);
                }
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                output = param_set;
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
            remaining = std::string(pos, end);
            temploid_reference param_set;
            param_set.templexoid = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "]"))
            {
                output = param_set;
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
            remaining = std::string(pos, end);
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                param_set.which.interface.named[param_name] = parse_argif(pos, end);
            }
            else
            {
                param_set.which.interface.positional.push_back(parse_argif(pos, end));
            }

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "]"))
            {
                output = param_set;
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
