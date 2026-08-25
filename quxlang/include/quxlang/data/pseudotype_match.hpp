// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_PSEUDOTYPE_MATCH_HEADER_GUARD
#define QUXLANG_DATA_PSEUDOTYPE_MATCH_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <map>
#include <string>

namespace quxlang
{
    /** Describes one exact structural match against an already-canonical type. */
    struct pseudotype_match_input
    {
        /// Pattern containing pseudotype placeholders.
        type_symbol pseudotype;
        /// Canonical candidate supplied by the caller.
        type_symbol type;

        RPNX_MEMBER_METADATA(pseudotype_match_input, pseudotype, type);
    };

    /** Contains the bindings produced by a pseudotype match. */
    struct pseudotype_match_result
    {
        /// Canonical type bound to each named pseudotype.
        std::map< std::string, type_symbol > matches;

        RPNX_MEMBER_METADATA(pseudotype_match_result, matches);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_PSEUDOTYPE_MATCH_HEADER_GUARD
