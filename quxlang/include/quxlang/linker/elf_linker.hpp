// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD
#define QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD

#include <quxlang/data/machine.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace quxlang
{
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
        auto link_linux_executable(machine_target_info const& machine, std::vector< std::byte > const& object_file, std::string const& entry_symbol) const -> std::vector< std::byte >;
    };
} // namespace quxlang

#endif // QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD
