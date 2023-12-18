//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef PARSE_NAMED_DECLARATIONS_HPP
#define PARSE_NAMED_DECLARATIONS_HPP
#include <rylang/parser.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>
#include <rylang/parsers/try_parse_named_declaration.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::vector< ast2_named_declaration > parse_named_declarations(It& pos, It end)
    {
        std::vector< ast2_named_declaration > output;

        while (true)
        {
            skip_whitespace_and_comments(pos, end);
            auto decl = try_parse_named_declaration(pos, end);
            if (!decl)
            {
                break;
            }
            output.push_back(*decl);
        }
        return output;
    }
} // namespace rylang::parsers

#endif // PARSE_NAMED_DECLARATIONS_HPP
