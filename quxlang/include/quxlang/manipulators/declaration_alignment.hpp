// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_DECLARATION_ALIGNMENT_HEADER_GUARD
#define QUXLANG_MANIPULATORS_DECLARATION_ALIGNMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <cstdint>

namespace quxlang
{
    /** Validates a constant alignment operand and returns its minimum byte alignment. */
    inline auto declaration_alignment_bytes(std::uint64_t value, bool is_exponent) -> std::uint64_t
    {
        if (is_exponent)
        {
            if (value > 32)
            {
                throw semantic_compilation_error("ALIGNBITS requires an exponent from 0 through 32");
            }
            return std::uint64_t(1) << value;
        }
        if (value == 0 || (value & (value - 1)) != 0 || value > (std::uint64_t(1) << 32))
        {
            throw semantic_compilation_error("ALIGN requires a nonzero power of two no greater than 4294967296");
        }
        return value;
    }
}

#endif // QUXLANG_MANIPULATORS_DECLARATION_ALIGNMENT_HEADER_GUARD
