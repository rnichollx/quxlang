// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_VIRTUAL_SLOTS_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_VIRTUAL_SLOTS_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Normalizes virtual declarations, overrides, and implicit destructor slots. */
    struct struct_virtual_slots_query
    {
        static constexpr auto query_id = "struct_virtual_slots";
        using input_type = type_symbol;
        using output_type = struct_virtual_slots;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_VIRTUAL_SLOTS_HEADER_GUARD
