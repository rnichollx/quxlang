// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_ELF_HPP
#define QUXLANG_ELF_HPP


#include <cstdint>

namespace quxlang
{

    /**
     * Enum class to represent the ELF file type.
     */
    enum class file_type : std::uint16_t
    {
        none = 0, // No file type
        relocatable = 1, // Relocatable file
        executable = 2, // Executable file
        dynamic = 3, // Shared object file
        core = 4 // Core file
        // Additional file types can be added here.
    };

    struct elf_header
    {
        /**
         * Magic number and other information for identifying the ELF format.
         * This includes the magic number 0x7F followed by "ELF" in ASCII, file class,
         * data encoding, and version among others. Specific bytes like EI_CLASS,
         * EI_DATA, and EI_VERSION can be represented as enum classes for further clarity.
         */
        std::uint8_t identification[16];

        /**
         * Specifies the object file type.
         */
        file_type type;

        /**
         * Specifies the required architecture for this file (e.g., x86_64, ARM).
         * This can also be represented by an enum class for known architectures.
         */
        uint16_t machine;

        /**
         * The version of the ELF specification to which this file conforms.
         * Potentially represented by an enum class for version numbers.
         */
        uint32_t version;

        /**
         * The virtual memory address to which the system first transfers control,
         * thus starting the process. If the file has no associated entry point, this field holds zero.
         */
        uint64_t entry_point_address;

        /**
         * Offset from the start of the file to the start of the program header table.
         * If the file has no program header table, this field holds zero.
         */
        uint64_t program_header_offset;

        /**
         * Offset from the start of the file to the start of the section header table.
         * If the file has no section header table, this field holds zero.
         */
        uint64_t section_header_offset;

        /**
         * Processor-specific flags associated with the file.
         */
        uint32_t processor_specific_flags;

        /**
         * The size, in bytes, of the ELF header.
         */
        uint16_t elf_header_size;

        /**
         * The size, in bytes, of an entry in the program header table. All entries
         * are the same size.
         */
        uint16_t program_header_entry_size;

        /**
         * The number of entries in the program header table. If there is no program
         * header table, this field holds zero.
         */
        uint16_t program_header_entry_count;

        /**
         * The size, in bytes, of an entry in the section header table. All entries
         * are the same size.
         */
        uint16_t section_header_entry_size;

        /**
         * The number of entries in the section header table. If there is no section
         * header table, this field holds zero.
         */
        uint16_t section_header_entry_count;

        /**
         * The section header table index of the entry associated with the section
         * name string table. If the file has no section name string table, this field
         * holds the value SHN_UNDEF (0).
         */
        uint16_t section_header_string_table_index;
    };

    template <typename It>
    elf64_header read_elf64_header(It begin, It end)
    {
    }

}

#endif //ELF_HPP