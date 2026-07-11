// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_STRUCT_TAGS_TYPES_HEADER_GUARD
#define QUXLANG_DATA_STRUCT_TAGS_TYPES_HEADER_GUARD

#include <set>
#include <string>

namespace quxlang
{
    /** Keyword tags attached to a struct declaration. */
    using struct_tags_result_type = std::set< std::string >;
} // namespace quxlang

#endif // QUXLANG_DATA_STRUCT_TAGS_TYPES_HEADER_GUARD
