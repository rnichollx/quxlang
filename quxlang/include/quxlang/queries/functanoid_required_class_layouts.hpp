// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_REQUIRED_CLASS_LAYOUTS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_REQUIRED_CLASS_LAYOUTS_HEADER_GUARD

#include <quxlang/data/functanoid_requirements.hpp>

#include <set>

namespace quxlang
{
    struct functanoid_required_class_layouts_query
    {
        static constexpr auto query_id = "functanoid_required_class_layouts";
        using input_type = functanoid_requirement_input;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_REQUIRED_CLASS_LAYOUTS_HEADER_GUARD
