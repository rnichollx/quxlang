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
        return make_artifact_output_path(build_dir, mangled_stem, ".functanoid.vmir2");
    }

    /**
     * Builds an input textual LLVM output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_input_llvm_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".functanoid.input.llvm");
    }

    /**
     * Builds a final textual LLVM output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_final_llvm_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".functanoid.final.llvm");
    }

    /**
     * Builds an input object output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_input_object_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".functanoid.input.o");
    }

    /**
     * Builds a final object output path whose individual filesystem components stay within the qxc filename budget.
     */
    inline auto make_final_object_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".functanoid.final.o");
    }

    /**
     * Builds an assembly text output path for one asm procedure.
     */
    inline auto make_asm_source_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".s");
    }

    /**
     * Builds an object output path for one standalone asm procedure.
     */
    inline auto make_asm_object_output_path(std::filesystem::path const& build_dir, std::string const& mangled_stem) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, mangled_stem, ".o");
    }

    /**
     * Builds an aggregated input output-module LLVM path for one qxc output entry.
     */
    inline auto make_output_module_input_llvm_output_path(std::filesystem::path const& build_dir, std::string const& output_name) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, output_name, ".module.input.llvm");
    }

    /**
     * Builds an aggregated final output-module LLVM path for one qxc output entry.
     */
    inline auto make_output_module_final_llvm_output_path(std::filesystem::path const& build_dir, std::string const& output_name) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, output_name, ".module.final.llvm");
    }

    /**
     * Builds an aggregated input output-module object path for one qxc output entry.
     */
    inline auto make_output_module_input_object_output_path(std::filesystem::path const& build_dir, std::string const& output_name) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, output_name, ".module.input.o");
    }

    /**
     * Builds an aggregated output-module final object path for one qxc output entry.
     */
    inline auto make_output_module_final_object_output_path(std::filesystem::path const& build_dir, std::string const& output_name) -> std::filesystem::path
    {
        return make_artifact_output_path(build_dir, output_name, ".module.final.o");
    }

}

#endif // QUXLANG_SOURCES_APP_QXC_OUTPUT_PATHS_HEADER_GUARD
