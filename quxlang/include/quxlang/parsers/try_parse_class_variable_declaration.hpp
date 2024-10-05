// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_CLASS_VARIABLE_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_CLASS_VARIABLE_DECLARATION_HEADER_GUARD
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/symbol.hpp>
#include <quxlang/parsers/try_parse_type_symbol.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< std::tuple< std::string, bool, ast2_variable_declaration > > try_parse_class_variable_declaration(It& pos, It end)
    {
        std::string start_str(pos, end);
        skip_whitespace_and_comments(pos, end);
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

        skip_whitespace(pos2, end);

        if (!skip_keyword_if_is(pos2, end, "VAR"))
        {
            return std::nullopt;
        }

        skip_whitespace_and_comments(pos2, end);

        type_symbol type = try_parse_type_symbol(pos2, end).value();

        std::string typestr = quxlang::to_string(type);

        skip_whitespace_and_comments(pos2, end);

        if (!skip_symbol_if_is(pos2, end, ";"))
        {
            throw std::logic_error("Expected ';' after VAR type");
        }

        pos = pos2;

        ast2_variable_declaration var;
        var.type = type;
        // TOOD: offset, include_if

        return {{name, is_member, var}};
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_CLASS_VARIABLE_DECLARATION_HPP
