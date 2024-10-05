//
// Created by Ryan Nicholl on 2/18/24.
//

#ifndef QUXLANG_DATA_MACHINE_HEADER_GUARD
#define QUXLANG_DATA_MACHINE_HEADER_GUARD

#include <cstddef>
#include <stdexcept>

#ifdef linux
#warning "Undefining linux"
#undef linux
#endif

#ifdef windows
#warning "Undefining windows"

#undef windows
#endif

namespace quxlang
{
    enum class cpu { x86_32, x86_64, arm_32, arm_64, riscv_32, riscv_64 };

    enum class os { none, linux, windows, macos, freebsd, netbsd, openbsd, solaris };

    enum class binary { elf, macho, pe, wasm };

    struct output_info
    {
        cpu cpu_type;
        os os_type;
        binary binary_type;

        constexpr inline std::size_t pointer_size() const
        {
            switch (cpu_type)
            {
            case cpu::x86_32:
            case cpu::arm_32:
            case cpu::riscv_32:
                return 4; //
            case cpu::x86_64:
            case cpu::arm_64:
            case cpu::riscv_64:
                return 8;
            }

            throw std::invalid_argument("pointer_size");
        };

        constexpr size_t max_int_align() const noexcept
        {
            return 8;
        }

        constexpr size_t pointer_align() const noexcept
        {
            return pointer_size();
        }
    };

} // namespace quxlang
#endif // RPNX_QUXLANG_MACHINE_HEADER
