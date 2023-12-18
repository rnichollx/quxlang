//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef RYLANG_PARSE_FILE_HPP
#define RYLANG_PARSE_FILE_HPP

#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/parsers/parse_named_declarations.hpp>
#include <rylang/parsers/try_parse_function_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    ast2_file_declaration parse_file(It& begin, It end)
    {
        ast2_file_declaration output;
        It& pos = begin;
        skip_wsc(pos, end);
        if (!skip_keyword_if_is(pos, end, "MODULE"))
        {
            throw std::runtime_error("expected module here");
        }

        skip_wsc(pos, end);

        std::string module_name = get_skip_identifier(pos, end);

        output.module_name = module_name;
        skip_wsc(pos, end);
        if (!skip_symbol_if_is(pos, end, ";"))
        {
            throw std::runtime_error("Expected ; here");
        }

        skip_wsc(pos, end);

        if (skip_keyword_if_is(pos, end, "IMPORT"))
        {
            skip_wsc(pos, end);
            std::string module_name = get_skip_identifier(pos, end);
            skip_wsc(pos, end);

            std::string import_name = module_name;

            if (skip_symbol_if_is(pos, end, "AS"))
            {
                skip_wsc(pos, end);
                import_name = get_skip_identifier(pos, end);
                skip_wsc(pos, end);
            }

            if (!skip_symbol_if_is(pos, end, ";"))
            {
                throw std::runtime_error("Expected ; here");
            }

            output.imports[import_name] = module_name;
        }

    entry:
        skip_whitespace_and_comments(pos, end);
        auto decl = parse_named_declarations(pos, end);

        for (auto d : decl)
        {
            if (typeis< ast2_named_global >(d))
            {
                auto element = as< ast2_named_global >(d);
                output.globals.push_back({element.name, element.declaration});
            }
        }

        return output;
    }

    ast2_file_declaration parse_file(std::string const& input)
    {
        auto it = input.begin();
        auto end = input.end();
        return parse_file(it, end);
    }
} // namespace rylang::parsers

#endif // PARSE_FILE_HPP
