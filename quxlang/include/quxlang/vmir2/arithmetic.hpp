// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_VMIR2_ARITHMETIC_HEADER_GUARD
#define QUXLANG_VMIR2_ARITHMETIC_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang::vmir2
{
    /** Reads the overflow contract of a scalar instruction; other instructions have no range failure. */
    template < typename Instruction >
    auto instruction_overflow_mode(Instruction const& instruction) -> overflow_mode
    {
        if constexpr (requires { instruction.overflow; })
        {
            return instruction.overflow;
        }
        return overflow_mode::warp;
    }

    /** Identifies the ordinary runtime call executed when an arithmetic range contract fails. */
    inline auto arithmetic_failure_function(overflow_mode mode) -> type_symbol
    {
        return instanciation_reference{
            .temploid = temploid_reference{
                .templexoid = subsymbol{.of = absolute_module_reference{.module_name = "RUNTIME"},
                    .name = mode == overflow_mode::checked ? "THROW_ARITHMETIC_OVERFLOW" : "PANIC_ARITHMETIC_OVERFLOW"},
            },
        };
    }
}

#endif
