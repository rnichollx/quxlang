// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_MACHINE_HEADER_GUARD
#define QUXLANG_DATA_MACHINE_HEADER_GUARD

#include <cstddef>
#include <cstdint>
#include <quxlang/exception.hpp>
#include <rpnx/macros.hpp>
#include <stdexcept>

#ifdef linux
// #warning "Undefining linux"
#undef linux
#endif

#ifdef windows
#warning "Undefining windows"

#undef windows
#endif

namespace quxlang
{
    enum class cpu { none, x86_32, x86_64, arm_32, arm_64, riscv_32, riscv_64 };

    enum class os { none, linux, windows, macos, freebsd, netbsd, openbsd, solaris };

    enum class binary { none, elf, macho, pe, wasm };

    struct machine_target_info
    {
        cpu cpu_type = cpu::none;
        os os_type = os::none;
        binary binary_type = binary::none;

        constexpr inline std::size_t pointer_size_bytes() const
        {
            switch (cpu_type)
            {
            case cpu::none:
                throw std::invalid_argument("Pointer size requires a CPU target");
            case cpu::x86_32:
            case cpu::arm_32:
            case cpu::riscv_32:
                return 4; //
            case cpu::x86_64:
            case cpu::arm_64:
            case cpu::riscv_64:
                return 8;
            }

            throw quxlang::compiler_bug("Pointer size is not implemented for this CPU");
        };

        constexpr std::uint64_t max_int_align() const noexcept
        {
            return 8;
        }

        /**
         * Returns the largest atomic storage width modeled as native for this target.
         */
        constexpr std::uint64_t max_native_atomic_storage_bits() const
        {
            switch (cpu_type)
            {
            case cpu::none:
                throw std::invalid_argument("Native atomic size requires a CPU target");
            case cpu::x86_32:
            case cpu::arm_32:
            case cpu::riscv_32:
                return 32;
            case cpu::x86_64:
            case cpu::arm_64:
            case cpu::riscv_64:
                return 64;
            }

            throw quxlang::compiler_bug("Native atomic size is not implemented for this CPU");
        }

        /**
         * Returns the target ABI alignment for an integer type with the given bit count.
         */
        constexpr std::uint64_t integer_alignment_for_bits(std::uint64_t bit_count) const
        {
            std::uint64_t byte_count = bit_count / 8;
            if (bit_count % 8 != 0)
            {
                byte_count += 1;
            }
            if (byte_count == 0)
            {
                byte_count = 1;
            }

            switch (cpu_type)
            {
            case cpu::none:
                throw std::invalid_argument("Integer alignment requires a CPU target");
            case cpu::x86_32:
            case cpu::arm_32:
            case cpu::riscv_32:
            {
                std::uint64_t alignment = 1;
                while (alignment * 2 <= byte_count && alignment * 2 <= 4)
                {
                    alignment *= 2;
                }
                return alignment;
            }
            case cpu::x86_64:
            case cpu::arm_64:
            case cpu::riscv_64:
            {
                std::uint64_t alignment = 1;
                while (alignment * 2 <= byte_count && alignment * 2 <= 8)
                {
                    alignment *= 2;
                }
                return alignment;
            }
            }

            throw quxlang::compiler_bug("Integer alignment is not implemented for this CPU");
        }

        /**
         * Returns the target-required alignment for an atomic integer type with the given bit count.
         */
        constexpr std::uint64_t atomic_integer_alignment_for_bits(std::uint64_t bit_count) const
        {
            std::uint64_t storage_byte_count = 1;
            while (storage_byte_count * 8 < bit_count)
            {
                storage_byte_count *= 2;
            }

            switch (cpu_type)
            {
            case cpu::none:
                throw std::invalid_argument("Atomic integer alignment requires a CPU target");
            case cpu::x86_32:
            case cpu::x86_64:
            case cpu::arm_32:
            case cpu::arm_64:
            case cpu::riscv_32:
            case cpu::riscv_64:
            {
                if (storage_byte_count > 16)
                {
                    throw quxlang::compiler_bug("Atomic integer alignment is not implemented for this bit width");
                }
                return storage_byte_count;
            }
            }

            throw quxlang::compiler_bug("Atomic integer alignment is not implemented for this CPU");
        }

        /**
         * Returns the target ABI alignment for a floating-point type with the given bit count.
         */
        constexpr std::uint64_t float_alignment_for_bits(std::uint64_t bit_count) const
        {
            std::uint64_t byte_count = bit_count / 8;
            if (bit_count % 8 != 0)
            {
                byte_count += 1;
            }
            if (byte_count == 0)
            {
                byte_count = 1;
            }

            switch (cpu_type)
            {
            case cpu::none:
                throw std::invalid_argument("Float alignment requires a CPU target");
            case cpu::x86_32:
            case cpu::arm_32:
            case cpu::riscv_32:
            {
                std::uint64_t alignment = 1;
                while (alignment * 2 <= byte_count && alignment * 2 <= 4)
                {
                    alignment *= 2;
                }
                return alignment;
            }
            case cpu::x86_64:
            case cpu::arm_64:
            case cpu::riscv_64:
            {
                std::uint64_t alignment = 1;
                while (alignment * 2 <= byte_count && alignment * 2 <= 8)
                {
                    alignment *= 2;
                }
                return alignment;
            }
            }

            throw quxlang::compiler_bug("Float alignment is not implemented for this CPU");
        }

        constexpr std::uint64_t pointer_align() const
        {
            return pointer_size_bytes();
        }

        RPNX_MEMBER_METADATA(machine_target_info, cpu_type, os_type, binary_type);
    };

} // namespace quxlang
#endif // RPNX_QUXLANG_MACHINE_HEADER
