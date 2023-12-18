//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_FUNCTION_DELEGATES_HPP
#define TRY_PARSE_FUNCTION_DELEGATES_HPP
#include <rylang/parsers/try_parse_delegate_callsite_args.hpp>
#include <rylang/parsers/try_parse_function_callsite_args.hpp>

namespace rylang::parsers
{
    template < typename It >
    std::vector< ast2_function_delegate > parse_function_delegates(It& pos, It end)
    {
        std::vector< ast2_function_delegate > output;

        if (skip_symbol_if_is(pos, end, ":>"))
        {
        delegate:
            skip_whitespace_and_comments(pos, end);

            ast2_function_delegate d;
            d.target = parse_type_symbol(pos, end);
            skip_whitespace_and_comments(pos, end);
            std::string remaning(pos, end);
            d.args = try_parse_delegate_callsite_args(pos, end).value();
            skip_whitespace_and_comments(pos, end);
            output.push_back(d);
            if (skip_symbol_if_is(pos, end, ","))
            {
                goto delegate;
            }
        }
        return output;
    }

} // namespace rylang::parsers

#endif // TRY_PARSE_FUNCTION_DELEGATES_HPP
