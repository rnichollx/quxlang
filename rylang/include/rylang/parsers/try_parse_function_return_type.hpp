//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_FUNCTION_RETURN_TYPE_HPP
#define TRY_PARSE_FUNCTION_RETURN_TYPE_HPP

#include <optional>
#include <rylang/data/qualified_symbol_reference.hpp>
#include <rylang/parsers/skip_whitespace.hpp>

namespace rylang::parsers
{

    template < typename It >
    std::optional< type_symbol > try_parse_function_return_type(It& pos, It end)
    {
        std::optional< type_symbol > out;
        skip_whitespace(pos, end);

        if (!skip_symbol_if_is(pos, end, ":"))
        {
            return out;
        }
        skip_whitespace(pos, end);

        out = parse_type_symbol(pos, end);
        return out;
    }
} // namespace rylang::parsers

#endif // TRY_PARSE_FUNCTION_RETURN_TYPE_HPP
