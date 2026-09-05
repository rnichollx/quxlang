// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_LINKER_PE_LINKER_HEADER_GUARD
#define QUXLANG_LINKER_PE_LINKER_HEADER_GUARD

#include <quxlang/data/machine.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace quxlang
{
    /** Describes one procedure imported through a Windows PE import table. */
    struct pe_dynamic_import
    {
        std::string symbol_name;
        std::string library_name;
        bool optional = false;
    };

    struct pe_link_options
    {
        /** Retains source debug sections and applies their relocations. */
        bool preserve_debug_information = false;
        std::vector< pe_dynamic_import > dynamic_imports;
    };

    /** Links LLVM COFF objects into a deterministic Windows PE executable. */
    class pe_linker
    {
    public:
        /**
         * Links independently compiled COFF objects into one Windows executable.
         *
         * Definitions in one object resolve undefined references in every other
         * object. LLVM weak externals and COFF COMDAT selection are resolved
         * before the retained sections are laid out in the final image.
         */
        auto link_windows_executable(machine_target_info const& machine,
                                     std::vector< std::vector< std::byte > > const& object_files,
                                     std::string const& entry_symbol,
                                     pe_link_options const& options = {}) const -> std::vector< std::byte >;
    };
} // namespace quxlang

#endif
