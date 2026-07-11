// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_HEADER_GUARD

#include <quxlang/data/functanoid_requirements.hpp>

#include <set>

namespace quxlang
{
    /** Lists the struct layouts required to emit a functanoid. */
    struct functanoid_required_struct_layouts_query
    {
        static constexpr auto query_id = "functanoid_required_struct_layouts";
        using input_type = functanoid_requirement_input;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_REQUIRED_STRUCT_LAYOUTS_HEADER_GUARD
