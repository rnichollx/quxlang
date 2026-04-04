// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_REQUIRES_GEN_COPY_CTOR_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_REQUIRES_GEN_COPY_CTOR_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct class_requires_gen_copy_ctor_query
    {
        static constexpr auto query_id = "class_requires_gen_copy_ctor";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_REQUIRES_GEN_COPY_CTOR_HEADER_GUARD
