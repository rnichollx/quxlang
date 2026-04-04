// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_DEFAULT_CTOR_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_DEFAULT_CTOR_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <optional>


namespace quxlang
{
    struct class_default_ctor_query
    {
        static constexpr auto query_id = "class_default_ctor";
        using input_type = type_symbol;
        using output_type = std::optional< instanciation_reference >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_DEFAULT_CTOR_HEADER_GUARD
