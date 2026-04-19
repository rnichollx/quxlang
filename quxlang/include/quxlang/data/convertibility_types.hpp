// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CONVERTIBILITY_TYPES_HEADER_GUARD
#define QUXLANG_DATA_CONVERTIBILITY_TYPES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct implicitly_convertible_to_input
    {
        type_symbol from;
        type_symbol to;

        RPNX_MEMBER_METADATA(implicitly_convertible_to_input, from, to);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_CONVERTIBILITY_TYPES_HEADER_GUARD
