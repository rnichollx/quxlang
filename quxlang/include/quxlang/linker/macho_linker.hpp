// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_LINKER_MACHO_LINKER_HEADER_GUARD
#define QUXLANG_LINKER_MACHO_LINKER_HEADER_GUARD

#include <quxlang/data/machine.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace quxlang
{
    /** Describes one procedure resolved from a loaded Mach-O dynamic library. */
    struct macho_dynamic_import
    {
        /** Symbol spelling used by relocatable input objects. */
        std::string relocation_symbol_name;
        /** Exported symbol spelling before the Mach-O C symbol prefix is added. */
        std::string symbol_name;
        /** Logical library selected by the Quxlang extern declaration. */
        std::string library_name;
        /** Allows dyld to leave the imported address null when the export is absent. */
        bool optional = false;
    };

    /** Controls metadata emitted into a final Mach-O executable. */
    struct macho_link_options
    {
        /** Identifier stored in the ad-hoc code signature. */
        std::string signature_identifier = "qxc-output";
        /** Procedures resolved by dyld when the executable is loaded. */
        std::vector< macho_dynamic_import > dynamic_imports;
    };

    /** Links LLVM Mach-O relocatable objects into a macOS executable image. */
    class macho_linker
    {
    public:
        /**
         * Links independently compiled Mach-O objects into one macOS executable.
         *
         * The entry symbol must be defined by an input object. ARM64 output is
         * ad-hoc signed so the resulting image can execute on Apple silicon.
         */
        auto link_macos_executable(machine_target_info const& machine,
                                   std::vector< std::vector< std::byte > > const& object_files,
                                   std::string const& entry_symbol,
                                   macho_link_options const& options = {}) const -> std::vector< std::byte >;
    };
} // namespace quxlang

#endif // QUXLANG_LINKER_MACHO_LINKER_HEADER_GUARD
