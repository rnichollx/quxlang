// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_PARSERS_TRY_PARSE_NAME_HEADER_GUARD
#define QUXLANG_PARSERS_TRY_PARSE_NAME_HEADER_GUARD

#include "quxlang/data/compilation_result.hpp"
#include "quxlang/parsers/keyword.hpp"
#include <quxlang/cpu_attributes.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace quxlang::parsers
{
    template < typename It >
    std::optional< std::pair< bool, std::string > > try_parse_name(It& pos, It end)
    {
        auto parse_runtime_entry_name = [](It& current, It finish) -> std::string
        {
            if (skip_keyword_if_is(current, finish, "PROGRAM_START"))
            {
                return "PROGRAM_START";
            }
            if (skip_keyword_if_is(current, finish, "POST_DETECT"))
            {
                return "POST_DETECT";
            }
            if (skip_keyword_if_is(current, finish, "UNIT_TEST_MAIN"))
            {
                return "UNIT_TEST_MAIN";
            }
            return {};
        };

        auto parse_cpu_attribute_detector_name = [](It& current, It finish) -> std::string
        {
            std::string const name = next_keyword(current, finish);
            constexpr std::string_view detect_prefix = "DETECT_";
            if (!name.starts_with(detect_prefix) ||
                !parse_cpu_attribute_stem(std::string_view(name).substr(detect_prefix.size())).has_value())
            {
                return {};
            }
            if (!skip_keyword_if_is(current, finish, name))
            {
                throw syntax_compilation_error("Expected CPU attribute detector name");
            }
            return name;
        };

        std::optional< std::pair< bool, std::string > > output;

        if (skip_symbol_if_is(pos, end, "."))
        {
            std::string name = parse_subentity(pos, end);
            if (name.empty())
            {
                name = parse_runtime_entry_name(pos, end);
            }
            output = {{true, std::move(name)}};
        }
        else if (skip_symbol_if_is(pos, end, "::"))
        {
            std::string name = parse_subentity(pos, end);
            if (name.empty())
            {
                name = parse_runtime_entry_name(pos, end);
            }
            if (name.empty())
            {
                name = parse_cpu_attribute_detector_name(pos, end);
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
