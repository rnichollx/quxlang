// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_BUILD_TYPE_HEADER_GUARD
#define QUXLANG_DATA_BUILD_TYPE_HEADER_GUARD

#include <cstdint>
#include <quxlang/data/compilation_result.hpp>
#include <string_view>
#include <utility>

/** Selects a named compilation policy independently of the compiler's own build. */
RPNX_ENUM(quxlang, build_type, std::uint8_t, debug, quick, release, debug_opt, debug_release, compact, debug_compact, compact_opt, debug_compact_opt);

namespace quxlang
{
    /** Parses build types independently of ASCII case and underscore separators. */
    inline auto parse_build_type(std::string_view name) -> build_type
    {
        std::string normalized;
        for (char character : name)
        {
            if (character == '_')
            {
                continue;
            }
            if (character >= 'A' && character <= 'Z')
            {
                character += 'a' - 'A';
            }
            normalized.push_back(character);
        }
        constexpr std::pair< std::string_view, build_type > names[] = {
            {"debug", build_type::debug}, {"quick", build_type::quick}, {"release", build_type::release}, {"debugopt", build_type::debug_opt}, {"debugrelease", build_type::debug_release}, {"compact", build_type::compact}, {"debugcompact", build_type::debug_compact}, {"compactopt", build_type::compact_opt}, {"debugcompactopt", build_type::debug_compact_opt},
        };
        for (std::pair< std::string_view, build_type > const& entry : names)
        {
            if (normalized == entry.first)
            {
                return entry.second;
            }
        }
        throw semantic_compilation_error("Unknown build_type: " + std::string(name));
    }

    /** Reports whether the policy emits and retains source debugging information. */
    inline auto build_type_has_debug_information(build_type value) -> bool
    {
        return value == build_type::debug || value == build_type::debug_opt || value == build_type::debug_release || value == build_type::debug_compact || value == build_type::debug_compact_opt;
    }

    /** Reports whether procedures are compiled in independent LLVM modules. */
    inline auto build_type_compiles_procedures(build_type value) -> bool
    {
        return value == build_type::debug || value == build_type::quick;
    }

    /** Reports whether all platform-default CPU steppings are enabled. */
    inline auto build_type_has_multiple_steppings(build_type value) -> bool
    {
        return value == build_type::release || value == build_type::debug_release;
    }
} // namespace quxlang
#endif
