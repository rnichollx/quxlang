//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_FUNCTION_DECLARATION_HEADER_GUARD

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

        It begin = pos;
        std::optional< ast2_function_declaration > out;

        if (!skip_keyword_if_is(pos, end, "FUNCTION"))
        {
            return out;
        }
        out = ast2_function_declaration{};

        // TODO: Add support for named arguments
        out->header.call_parameters = parse_function_args(pos, end);
        // TODO: Parse enable if and other attributes

        out->definition.return_type = try_parse_function_return_type(pos, end);
        out->definition.delegates = parse_function_delegates(pos, end);
        out->definition.body = parse_function_block(pos, end);
        out->location.set(begin, pos);
        return out;
    }
} // namespace quxlang::parsers

#endif // TRY_PARSE_FUNCTION_DECLARATION_HPP
