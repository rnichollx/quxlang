// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_CLASS_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_CLASS_FUNCTION_DECLARATION_HEADER_GUARD
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>

#include <quxlang/parsers/try_parse_function_declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< std::tuple< std::string, bool, ast2_function_declaration > > try_parse_class_function_declaration(It& pos, It end)
    {
        auto pos2 = pos;

        bool is_member;

        if (skip_symbol_if_is(pos2, end, "."))
        {
            is_member = true;
        }
        else if (skip_symbol_if_is(pos2, end, "::"))
        {
            is_member = false;
        }
        else
        {
            return std::nullopt;
        }
        std::string name = parse_identifier(pos2, end);

        if (name.empty())
        {
            throw std::logic_error("Expected identifier");
        }

        skip_whitespace_and_comments(pos2, end);

        if (!skip_keyword_if_is(pos2, end, "FUNCTION"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos2, end);

        auto function_opt = try_parse_function_declaration(pos2, end);
        if (!function_opt)
        {
            return std::nullopt;
        }

        auto function = *function_opt;

        pos = pos2;

        return { { name, is_member, function } };
    }

} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_FUNCTION_DECLARATION_HPP
