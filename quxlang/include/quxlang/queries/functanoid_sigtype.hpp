// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_SIGTYPE_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_SIGTYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    struct functanoid_sigtype_query
    {
        static constexpr auto query_id = "functanoid_sigtype";
        using input_type = instanciation_reference;
        using output_type = sigtype;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_SIGTYPE_HEADER_GUARD
