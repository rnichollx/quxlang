// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_MANIPULATORS_CODE_ALIGNMENT_HEADER_GUARD
#define QUXLANG_MANIPULATORS_CODE_ALIGNMENT_HEADER_GUARD

#include <quxlang/data/compilation_result.hpp>
#include <cstdint>

namespace quxlang
{
    /** Validates byte alignment within the compiler's supported alignment range. */
    inline void validate_code_alignment(std::uint64_t alignment)
    {
        if (alignment == 0 || (alignment & (alignment - 1)) != 0 || alignment > (std::uint64_t(1) << 31))
        {
            throw semantic_compilation_error("HINT_CODE_ALIGN requires a nonzero power of two no greater than 2147483648");
        }
    }
}
#endif
