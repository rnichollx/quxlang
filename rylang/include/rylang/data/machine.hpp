//
// Created by Ryan Nicholl on 2/18/24.
//

#ifndef RPNX_QUXLANG_MACHINE_HEADER
#define RPNX_QUXLANG_MACHINE_HEADER

#include <cstddef>

#ifdef linux
#warning "Undefining linux"
#undef linux
#endif

#ifdef windows
#warning "Undefining windows"

#undef windows
#endif

namespace rylang
{
    enum class cpu { x86_32, x86_64, arm_32, arm_64, riscv_32, riscv_64 };

    enum class os { linux, windows, macos, freebsd, netbsd, openbsd, solaris, aix, vxworks, qnx };

    enum class binary { elf, macho, pe, wasm };

    struct machine_info
    {
        cpu cpu;
        os os;
        binary binary;

        constexpr inline std::size_t pointer_size() const noexcept
        {
            switch (cpu)
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
} // namespace rylang
#endif // RPNX_QUXLANG_MACHINE_HEADER
