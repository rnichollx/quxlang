//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_NAMED_DECLARATION_HPP
#define TRY_PARSE_NAMED_DECLARATION_HPP
#include <rylang/ast2/ast2_named_declaration.hpp>
#include <rylang/parsers/parse_declaration.hpp>
#include <rylang/parsers/try_parse_name.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::optional< ast2_named_declaration > try_parse_named_declaration(It& pos, It end)
    {
        std::optional< ast2_named_declaration > output;

        auto name_opt = try_parse_name(pos, end);
        if (!name_opt)
        {
            return output;
        }
        auto [member, name] = *name_opt;

        skip_whitespace_and_comments(pos, end);
        std::string remaning(pos, end);

        auto decl = parse_declaration(pos, end);

        if (member)
        {
            ast2_named_member m{};
            m.name = name;
            m.declaration = decl;
            output = m;
        }
        else
        {
            ast2_named_global g{};
            g.name = name;
            g.declaration = decl;
            output = g;
        }
        return output;
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_NAMED_DECLARATION_HPP
