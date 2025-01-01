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
    type_symbol parse_type_symbol(It& pos, It end);

    template < typename It >
    std::optional< type_symbol > try_parse_type_symbol(It& pos, It end)
    {
        type_symbol output = context_reference{};
        std::string remaining = std::string(pos, end);
        skip_whitespace_and_comments(pos, end);
    start:
        skip_whitespace(pos, end);
        if (skip_keyword_if_is(pos, end, "NUMERIC_LITERAL"))
        {
            output = numeric_literal_reference{};
        }
        else if (skip_keyword_if_is(pos, end, "VOID"))
        {
            output = void_type{};
        }
        else if (skip_keyword_if_is(pos, end, "BOOL"))
        {
            output = bool_type{};
        }
        else if (auto int_kw = try_parse_integral_keyword(pos, end); int_kw)
        {
            output = *int_kw;
        }
        else if (skip_keyword_if_is(pos, end, "T"))
        {
            template_reference tref;

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
        else if (skip_keyword_if_is(pos, end, "MUT"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after MUT");
            }
            return mvalue_reference{parse_type_symbol(pos, end)};
        }
        else if (skip_keyword_if_is(pos, end, "CONST"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after MUT");
            }
            return cvalue_reference{parse_type_symbol(pos, end)};
        }
        else if (skip_keyword_if_is(pos, end, "WRITE"))
        {
            if (!skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after WRITE");
            }
            return wvalue_reference{parse_type_symbol(pos, end)};
        }
        else if (skip_keyword_if_is(pos, end, "TEMP"))
        {
            if (skip_symbol_if_is(pos, end, "&"))
            {
                // TODO: Support MUT-> etc
                throw std::logic_error("Expected & after MUT");
            }
            return tvalue_reference{parse_type_symbol(pos, end)};
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
                throw std::logic_error("Expected & after DESTROY");
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
            return auto_reference{parse_type_symbol(pos, end)};
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            // TODO: Support multiple modules
            output = module_reference{"main"};

            auto ident = parse_subentity(pos, end);
            if (ident.empty())
                throw std::logic_error("expected identifier after ::");

            output = subsymbol{std::move(output), std::move(ident)};
        }
        else if (skip_symbol_if_is(pos, end, "."))
        {
            std::string remaining = std::string(pos, end);
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = submember{std::move(output), std::move(ident)};
        }
        else if (skip_symbol_if_is(pos, end, "->"))
        {
            return pointer_type{parse_type_symbol(pos, end)};
        }
        else
        {
            std::string remaining = std::string(pos, end);
            auto ident = parse_subentity(pos, end);
            if (ident.empty())
            {
                return std::nullopt;
            }
            output = subsymbol{std::move(output), std::move(ident)};
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
            instantiation_type param_set;
            param_set.callee = std::move(output);

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

            instantiation_type param_set;
            param_set.callee = temploid_reference{};

            temploid_reference& sel = as< temploid_reference >(param_set.callee);
            sel.templexoid = std::move(output);

            skip_whitespace_and_comments(pos, end);
            if (skip_symbol_if_is(pos, end, "}"))
            {
                output = param_set;
                goto check_next;
            }

            skip_whitespace_and_comments(pos, end);

            if (skip_keyword_if_is(pos, end, "BUILTIN"))
            {
                sel.which.builtin = true;

                skip_whitespace(pos, end);

                if (!skip_symbol_if_is(pos, end, ";"))
                {
                    throw std::logic_error("Expected ';'");
                }
            }

            skip_whitespace_and_comments(pos, end);
        next_arg2:
            remaining = std::string(pos, end);
            skip_whitespace_and_comments(pos, end);

            if (skip_symbol_if_is(pos, end, "@"))
            {
                std::string param_name = parse_argument_name(pos, end);
                auto seltype = parse_type_symbol(pos, end);
                sel.which.interface.named[param_name] = seltype;
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    param_set.parameters.named[param_name] = parse_type_symbol(pos, end);
                }
                else
                {
                    param_set.parameters.named[param_name] = seltype;
                }
            }
            else
            {
                auto seltype = parse_type_symbol(pos, end);
                sel.which.interface.positional.push_back(seltype);
                skip_whitespace(pos, end);
                if (skip_symbol_if_is(pos, end, ":"))
                {
                    param_set.parameters.positional.push_back(parse_type_symbol(pos, end));
                }
                else
                {
                    param_set.parameters.positional.push_back(seltype);
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
                param_set.which.builtin = true;

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
                param_set.which.interface.named[param_name] = parse_type_symbol(pos, end);
            }
            else
            {
                param_set.which.interface.positional.push_back(parse_type_symbol(pos, end));
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

#endif // TRY_PARSE_TYPE_SYMBOL_HPP
