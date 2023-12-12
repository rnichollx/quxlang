//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_FUNCTION_DECLARATION_HPP
#define TRY_PARSE_FUNCTION_DECLARATION_HPP

#include <optional>

#include <rylang/ast2/ast2_function_declaration.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>

#include <rylang/parsers/try_parse_function_body.hpp>

namespace rylang::parsers
{
    std::optional< ast2_function_declaration > try_parse_function_declaration(std::string_view& pos, std::string_view end)
    {
        std::optional< ast2_function_declaration > out;

        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            return out;
        }

        out = parse_function_body(pos, end);
        return out;
    }
} // namespace rylang

#endif // TRY_PARSE_FUNCTION_DECLARATION_HPP
