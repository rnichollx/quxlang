// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_GENERIC_CONTRACT_HEADER_GUARD
#define QUXLANG_DATA_GENERIC_CONTRACT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <set>

namespace quxlang
{
    /** One unresolved edge in the transitive generic contract graph. */
    struct generic_implements_entry
    {
        type_symbol context;
        type_symbol declared_type;
        std::set< type_symbol > ancestry;
    };
}

#endif // QUXLANG_DATA_GENERIC_CONTRACT_HEADER_GUARD
