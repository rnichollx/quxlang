// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD
#define QUXLANG_LINKER_ELF_LINKER_HEADER_GUARD

#include <cstdlib>
namespace quxlang
{

    namespace elf
    {

        struct program_header
        {
            struct flags
            {
                bool read;
                bool write;
                bool execute;
            };

            enum class segment_type
            {
                null,
                loadable,
                dynamic,
                interpreter,
                note,
                program_header_loc,
                thread_local_storage,
                gnu_exception_frame,
                gnu_stack,
                gnu_relocate_readonly,
            };

            std::size_t offset;
            std::size_t virtual_address;
            std::size_t physical_address;
            std::size_t memory_size;

        };
    } // namespace elf

    class elf_linker
    {

      public:
        elf_linker();
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_ELF_LINKER_HEADER
