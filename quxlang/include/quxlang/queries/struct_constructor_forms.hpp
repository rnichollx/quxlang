// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_CONSTRUCTOR_FORMS_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_CONSTRUCTOR_FORMS_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Normalizes constructor templates and explicit full/subobject pairs. */
    struct struct_constructor_forms_query
    {
        static constexpr auto query_id = "struct_constructor_forms";
        using input_type = type_symbol;
        using output_type = struct_constructor_forms;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_CONSTRUCTOR_FORMS_HEADER_GUARD
