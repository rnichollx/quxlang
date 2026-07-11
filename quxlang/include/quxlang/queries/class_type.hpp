// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_TYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_type.hpp>

namespace quxlang
{
    /** Classifies a constructible type without changing its general symbol kind. */
    struct class_type_query
    {
        static constexpr auto query_id = "class_type";
        using input_type = type_symbol;
        using output_type = class_kind;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_TYPE_HEADER_GUARD
