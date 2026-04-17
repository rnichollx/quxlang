// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD

#include <quxlang/parsers/context.hpp>
#include "declaration.hpp"
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>

namespace quxlang::parsers
{
    ast2_file_declaration parse_file2(parsing_context ctx);

    inline ast2_file_declaration parse_file(parsing_context& ctx)
    {
        auto& pos = ctx.iter_pos;
        auto end = ctx.iter_end;
        auto begin = ctx.iter_pos;
        ast2_file_declaration output;
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

        skip_whitespace_and_comments(pos, end);
        auto decl = parse_subdeclaroids(ctx);
        skip_whitespace_and_comments(pos, end);

        for (auto d : decl)
        {
            output.declarations.push_back(d);
        }

        if (pos != end)
        {
            throw std::logic_error("Expected parse_subdeclaroids to consume the remainder of the file");
        }

        output.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return output;
    }

} // namespace quxlang::parsers

#endif // PARSE_FILE_HPP
