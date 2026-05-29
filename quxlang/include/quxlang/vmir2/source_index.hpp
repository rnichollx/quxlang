// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_VMIR2_SOURCE_INDEX_HEADER_GUARD
#define QUXLANG_VMIR2_SOURCE_INDEX_HEADER_GUARD

#include <quxlang/ast2/source_location.hpp>
#include <quxlang/data/target_configuration.hpp>

#include <map>
#include <optional>
#include <rpnx/macros.hpp>
#include <string>
#include <vector>

namespace quxlang::vmir2
{
    struct source_position
    {
        std::size_t line = 1;
        std::size_t column = 1;

        RPNX_MEMBER_METADATA(source_position, line, column);
    };

    struct indexed_source_file
    {
        source_file_name name;
        std::string contents;
        std::vector< std::size_t > line_starts;

        indexed_source_file() = default;
        indexed_source_file(source_file_name name, std::string contents);

        [[nodiscard]] auto path() const -> std::string;
        [[nodiscard]] auto position(std::size_t offset) const -> source_position;

        RPNX_MEMBER_METADATA(indexed_source_file, name, contents, line_starts);
    };

    struct source_index
    {
        std::map< std::uint64_t, indexed_source_file > files;

        source_index() = default;
        source_index(source_file_index const& file_index, source_bundle const& bundle);

        [[nodiscard]] auto format(std::optional< source_location > const& location) const -> std::string;

        RPNX_MEMBER_METADATA(source_index, files);
    };
} // namespace quxlang::vmir2

#endif // QUXLANG_VMIR2_SOURCE_INDEX_HEADER_GUARD
