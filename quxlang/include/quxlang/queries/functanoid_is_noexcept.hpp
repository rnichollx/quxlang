// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_FUNCTANOID_IS_NOEXCEPT_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_IS_NOEXCEPT_HEADER_GUARD
#include <quxlang/data/basic_types.hpp>
namespace quxlang
{
    /** Reports the exception contract of a functanoid. */
    struct functanoid_is_noexcept_query
    {
        static constexpr auto query_id = "functanoid_is_noexcept";
        using input_type = instanciation_reference;
        using output_type = bool;
    };
}
#endif
