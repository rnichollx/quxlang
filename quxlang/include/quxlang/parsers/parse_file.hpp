// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD

#include "declaration.hpp"
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <quxlang/parsers/declaration.hpp>

namespace quxlang::parsers
{
    template < typename It >
    ast2_file_declaration parse_file(It& begin, It end)
    {
        ast2_file_declaration output;
        It& pos = begin;
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "MODULE"))
        {
            throw std::logic_error("expected module here");
        }

        skip_whitespace_and_comments(pos, end);

        std::string module_name = parse_identifier(pos, end);

        output.module_name = module_name;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::logic_error("Expected ; here");
        }

        skip_whitespace_and_comments(pos, end);

        if (skip_keyword_if_is(pos, end, "IMPORT"))
        {
            skip_whitespace_and_comments(pos, end);
            std::string module_name = parse_identifier(pos, end);
            skip_whitespace_and_comments(pos, end);

            std::string import_name = module_name;

            if (skip_symbol_if_is(pos, end, "AS"))
            {
                skip_whitespace_and_comments(pos, end);
                import_name = parse_identifier(pos, end);
                skip_whitespace_and_comments(pos, end);
            }

            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::logic_error("Expected ; here");
            }

            output.imports[import_name] = module_name;
        }

    entry:
        skip_whitespace_and_comments(pos, end);
        auto decl = parse_subdeclaroids(pos, end);
        skip_whitespace_and_comments(pos, end);

        for (auto d : decl)
        {
            output.declarations.push_back(d);
        }

        if (pos != end)
        {
            throw std::logic_error("Expected parse_subdeclaroids to consume the remainder of the file");
        }

        return output;
    }

    inline ast2_file_declaration parse_file(std::string const& input)
    {
        auto it = input.begin();
        auto end = input.end();
        return parse_file(it, end);
    }
} // namespace quxlang::parsers

#endif // PARSE_FILE_HPP
