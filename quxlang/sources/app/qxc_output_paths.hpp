// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_APP_QXC_OUTPUT_PATHS_HEADER_GUARD
#define QUXLANG_SOURCES_APP_QXC_OUTPUT_PATHS_HEADER_GUARD

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <string>

namespace quxlang::qxc_detail
{
    inline constexpr std::size_t max_vmir2_path_component_length = 200;
    inline constexpr std::size_t max_vmir2_dir_stem_length = max_vmir2_path_component_length - 4;
    inline constexpr std::size_t max_vmir2_file_stem_length = max_vmir2_path_component_length - 6;

    /**
     * Builds an output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_artifact_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem, std::string const& extension) -> std::filesystem::path
    {
        std::size_t const max_file_stem_length = max_vmir2_path_component_length - extension.size();

        if (mangled_stem.size() <= max_file_stem_length)
        {
            return build_dir / (mangled_stem + extension);
        }

        std::filesystem::path result = build_dir;
        std::size_t offset = 0;
        while (mangled_stem.size() - offset > max_file_stem_length)
        {
            std::size_t const remaining = mangled_stem.size() - offset;
            std::size_t const dir_chunk_length = std::min(max_vmir2_dir_stem_length, remaining - max_file_stem_length);
            result /= mangled_stem.substr(offset, dir_chunk_length) + ".dir";
            offset += dir_chunk_length;
        }

        result /= mangled_stem.substr(offset) + extension;
        return result;
    }

    /**
     * Builds a VMIR2 output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_vmir2_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".vmir2");
    }

    /**
     * Builds a textual LLVM output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_llvm_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".dbg.llvm");
    }

    /**
     * Builds an optimized textual LLVM output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_optimized_llvm_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".opt.llvm");
    }
}

#endif // QUXLANG_SOURCES_APP_QXC_OUTPUT_PATHS_HEADER_GUARD
