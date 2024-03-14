//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef TRY_PARSE_FUNCTION_DECLARATION_HPP
#define TRY_PARSE_FUNCTION_DECLARATION_HPP

#include <optional>

#include <quxlang/ast2/ast2_function_delegate.hpp>
#include <quxlang/parsers/parse_function_args.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/parsers/keyword.hpp>
#include <quxlang/parsers/try_parse_function_delegates.hpp>
#include <quxlang/parsers/try_parse_function_return_type.hpp>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< ast2_function_declaration > try_parse_function_declaration(It& pos, It end)
    {
        std::string str (pos, end);
        std::optional< ast2_function_declaration > out;

        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            return out;
        }
        out = ast2_function_declaration{};

        out->args = parse_function_args(pos, end);
        out->return_type = try_parse_function_return_type(pos, end);

        out->delegates = parse_function_delegates(pos, end);
        out->body = parse_function_block(pos, end);
        return out;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_DECLARATION_HPP
