// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCE_LOADER_INTERNAL_HEADER_GUARD
#define QUXLANG_SOURCE_LOADER_INTERNAL_HEADER_GUARD

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace quxlang::detail
{
    /**
     * Returns an ASCII-case-folded path key without consulting the process
     * locale.
     */
    auto portable_case_fold(std::string_view value) -> std::string;

    /** Returns whether a filename byte is forbidden by a major filesystem. */
    auto is_illegal_portable_filename_byte(unsigned char character) -> bool;

    /** Validates one filename component against portable filesystem limits. */
    void validate_filename_component(std::string_view relative_path, std::string_view component);

    /** Returns whether source contents contain a recognized Unicode byte-order mark. */
    auto contains_byte_order_mark(std::string_view contents) -> bool;

    /**
     * Validates source-bundle paths and detects names which collide on a
     * case-insensitive filesystem.
     */
    class source_path_validator
    {
      public:
        /** Validates and records one path relative to the source-bundle root. */
        void add(std::filesystem::path const& relative_path);

      private:
        std::map< std::string, std::string > m_case_folded_paths;
    };

    /** Validates source text for byte sequences which make checkout-dependent input possible. */
    void validate_source_file_contents(std::string_view relative_path, std::string_view contents);
} // namespace quxlang::detail

#endif // QUXLANG_SOURCE_LOADER_INTERNAL_HEADER_GUARD
