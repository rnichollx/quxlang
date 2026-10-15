// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_DATA_MACHO_CONFIGURATION_HEADER_GUARD
#define QUXLANG_DATA_MACHO_CONFIGURATION_HEADER_GUARD

#include <cstdint>
#include <rpnx/macros.hpp>

namespace quxlang
{
    /** Represents a Mach-O operating system version without losing component boundaries. */
    struct macho_version
    {
        std::uint16_t major = 11;
        std::uint8_t minor = 0;
        std::uint8_t patch = 0;

        RPNX_MEMBER_METADATA(macho_version, major, minor, patch);
    };

    /** Configures Mach-O metadata for all outputs of a target. */
    struct macho_configuration
    {
        /** Oldest operating system version supported by the output. */
        macho_version minimum_os_version;

        RPNX_MEMBER_METADATA(macho_configuration, minimum_os_version);
    };
}

#endif
