// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD
#define QUXLANG_PARSERS_PARSE_FILE_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"

#include <quxlang/parsers/context.hpp>
#include "declaration.hpp"
#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/parsers/declaration.hpp>
#include <quxlang/parsers/iter_parse_number.hpp>
#include <quxlang/parsers/try_parse_function_declaration.hpp>
#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

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

        if (!skip_keyword_if_is(pos, end, "LANGUAGE"))
        {
            throw syntax_compilation_error("Expected LANGUAGE declaration at start of file");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "QUXLANG"))
        {
            throw syntax_compilation_error("Expected QUXLANG after LANGUAGE");
        }
        skip_whitespace_and_comments(pos, end);
        if (!skip_keyword_if_is(pos, end, "EN"))
        {
            throw syntax_compilation_error("Expected EN after LANGUAGE QUXLANG");
        }
        skip_whitespace_and_comments(pos, end);
        std::string_view language_version = "0.0";
        auto version_end = iter_parse_number(pos, end);
        if (version_end == pos || static_cast< std::size_t >(std::distance(pos, version_end)) != language_version.size() ||
            !std::equal(pos, version_end, language_version.begin(), language_version.end()))
        {
            throw syntax_compilation_error("Expected language version 0.0");
        }
        pos = version_end;
        skip_whitespace_and_comments(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw syntax_compilation_error("Expected ';' after LANGUAGE declaration");
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
                throw syntax_compilation_error("Expected ; here");
            }

            output.imports.emplace(std::move(import_name), std::move(module_name));
        }

        skip_whitespace_and_comments(pos, end);
        auto decl = parse_subdeclaroids(ctx);
        skip_whitespace_and_comments(pos, end);

        output.declarations = std::move(decl);

        if (pos != end)
        {
            throw syntax_compilation_error("Expected parse_subdeclaroids to consume the remainder of the file");
        }

        output.location = ctx.get_location_optional(begin, ctx.iter_pos);
        return output;
    }

} // namespace quxlang::parsers

#endif // PARSE_FILE_HPP
