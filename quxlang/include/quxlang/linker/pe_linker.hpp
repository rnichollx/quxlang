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
        std::vector< pe_dynamic_import > dynamic_imports;
    };

    /** Links one LLVM COFF object into a deterministic Windows PE executable. */
    class pe_linker
    {
    public:
        auto link_windows_executable(machine_target_info const& machine,
                                     std::vector< std::byte > const& object_file,
                                     std::string const& entry_symbol,
                                     pe_link_options const& options = {}) const -> std::vector< std::byte >;
    };
} // namespace quxlang

#endif
