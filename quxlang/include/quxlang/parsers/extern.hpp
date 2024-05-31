// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef EXTERN_HPP
#define EXTERN_HPP

#include "parse_whitespace_and_comments.hpp"
#include "symbol.hpp"
#include "string_literal.hpp"
#include "quxlang/ast2/ast2_entity.hpp"

namespace quxlang::parsers
{
    template <typename It>
    std::optional< ast2_extern > try_parse_ast2_extern(It& it, It end)
    {
        QUXLANG_DEBUG({std::cout << "try_parse_ast2_extern" << std::endl;});
        auto pos = it;
        skip_whitespace_and_comments(pos, end);

        if (!skip_keyword_if_is(pos, end, "EXTERNAL"))
        {
            return std::nullopt;
        }

        if (!skip_symbol_if_is(pos, end, "("))
        {
            throw std::runtime_error("Expected '(' after EXTERNAL");
        }

        skip_whitespace_and_comments(pos, end);

        auto lang = try_parse_string_literal(pos, end);

        if (!lang)
        {
            throw std::runtime_error("Expected language string after EXTERNAL(");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ","))
        {
            throw std::runtime_error("Expected ',' after EXTERNAL(\"" + std::string(*lang) + "\"");
        }

        skip_whitespace_and_comments(pos, end);

        auto name = try_parse_string_literal(pos, end);

        if (!name)
        {
            throw std::runtime_error("Expected name string after EXTERNAL(\"" + std::string(*lang) + "\",");
        }

        skip_whitespace_and_comments(pos, end);

        if (!skip_symbol_if_is(pos, end, ")"))
        {
            throw std::runtime_error("Expected ')' after EXTERNAL(\"" + std::string(*lang) + "\", \"" + std::string(*name) + "\"");
        }

        return ast2_extern{
            .lang = *lang,
            .symbol = *name
        };
    }

}

#endif //EXTERN_HPP