// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_NAME_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_NAME_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/parsers/keyword.hpp"
#include <string>
#include <utility>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< std::pair< bool, std::string > > try_parse_name(It& pos, It end)
    {
        std::optional< std::pair< bool, std::string > > output;

        if (skip_symbol_if_is(pos, end, "."))
        {
            std::string name = parse_subentity(pos, end);
            if (name.empty() && skip_keyword_if_is(pos, end, "PROGRAM_START"))
            {
                name = "PROGRAM_START";
            }
            output = {{true, std::move(name)}};
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            std::string name = parse_subentity(pos, end);
            if (name.empty() && skip_keyword_if_is(pos, end, "PROGRAM_START"))
            {
                name = "PROGRAM_START";
            }
            output = {{false, std::move(name)}};
        }

        if (output.has_value() && output->second.empty())
        {
            throw syntax_compilation_error("Expected identifier");
        }

        return output;
    }
} // namespace quxlang

#endif // TRY_PARSE_NAME_HPP
