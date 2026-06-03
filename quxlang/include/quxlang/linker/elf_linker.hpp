// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD
#define QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD

#include <quxlang/data/machine.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace quxlang
{
    /**
     * elf_link_options controls optional metadata emitted into the final ELF image.
     */
    struct elf_link_options
    {
        bool preserve_symbols = false;

        /**
         * Maps raw object-file symbol names to names written in the output ELF symbol table.
         */
        std::map< std::string, std::string > symbol_display_names;
    };

    /**
     * elf_linker links one Linux ELF relocatable object into one standalone executable image.
     *
     * The linker consumes the object bytes in memory and returns the final ELF file bytes
     * without creating intermediate files.
     */
    class elf_linker
    {
    public:
        /**
         * Links one Linux ELF executable from the provided relocatable object bytes.
         *
         * The entry symbol must be defined by the object, typically `_start`.
         */
        auto link_linux_executable(machine_target_info const& machine,
                                   std::vector< std::byte > const& object_file,
                                   std::string const& entry_symbol,
                                   elf_link_options const& options = {}) const -> std::vector< std::byte >;
    };
} // namespace quxlang

#endif // QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD
