//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_FUNCTION_DECLARATION_HPP
#define TRY_PARSE_FUNCTION_DECLARATION_HPP

#include <optional>

#include <rylang/ast2/ast2_function_declaration.hpp>
#include <rylang/parsers/parse_function_block.hpp>
#include <rylang/parsers/skip_keyword_if_is.hpp>
#include <rylang/parsers/try_parse_function_return_type.hpp>
#include <rylang/parsers/parse_function_args.hpp>
#include <rylang/parsers/parse_function_block.hpp>

namespace rylang::parsers
{
    template <typename It>
    std::optional< ast2_function_declaration > try_parse_function_declaration(It & pos, It end)
    {
        std::optional< ast2_function_declaration > out;

        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            return out;
        }
        out = ast2_function_declaration{};

        out->args = parse_function_args(pos, end);
        out->return_type = try_parse_function_return_type(pos, end);


        out->body = parse_function_block(pos, end);
        return out;
    }
} // namespace rylang

#endif // TRY_PARSE_FUNCTION_DECLARATION_HPP
