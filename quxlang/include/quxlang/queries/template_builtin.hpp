// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLATE_BUILTIN_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLATE_BUILTIN_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct template_builtin_query
    {
        static constexpr auto query_id = "template_builtin";
        using input_type = temploid_reference;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLATE_BUILTIN_HEADER_GUARD
