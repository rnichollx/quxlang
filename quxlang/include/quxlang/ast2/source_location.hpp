// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_SOURCE_LOCATION_HEADER_GUARD
#define QUXLANG_AST2_SOURCE_LOCATION_HEADER_GUARD
#include <rpnx/macros.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace quxlang
{

    struct source_location
    {
        std::uint64_t file_id = {};
        std::size_t begin_index = {};
        std::optional<std::size_t> end_index = {};

        RPNX_MEMBER_METADATA(source_location, file_id, begin_index, end_index);
    };

    inline std::string to_string(source_location const& location)
    {
        std::string result = "@@(" + std::to_string(location.file_id) + ", " + std::to_string(location.begin_index);
        if (location.end_index.has_value())
        {
            result += ", " + std::to_string(*location.end_index);
        }
        result += ")";
        return result;
    }

    inline std::string source_location_suffix(std::optional< source_location > const& location)
    {
        if (!location.has_value())
        {
            return {};
        }
        return " " + to_string(*location);
    }
}

#endif //SOURCE_LOCATION_H
