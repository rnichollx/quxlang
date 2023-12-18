//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TRY_PARSE_NAME_HPP
#define TRY_PARSE_NAME_HPP
#include <string>

namespace rylang::parsers
{
    template < typename It >
    std::optional< std::pair< bool, std::string > > try_parse_name(It& pos, It end)
    {
        std::optional< std::pair< bool, std::string > > output;

        if (skip_symbol_if_is(pos, end, "."))
        {
            output = {{true, parse_subentity(pos, end)}};
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            output = {{false, parse_subentity(pos, end)}};
        }

        if (output.has_value() && output->second.empty())
        {
            throw std::runtime_error("Expected identifier");
        }

        return output;
    }
} // namespace rylang

#endif // TRY_PARSE_NAME_HPP
