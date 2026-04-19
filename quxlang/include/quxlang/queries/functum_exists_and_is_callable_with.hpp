// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    struct functum_exists_and_is_callable_with_query
    {
        static constexpr auto query_id = "functum_exists_and_is_callable_with";
        using input_type = initialization_reference;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_EXISTS_AND_IS_CALLABLE_WITH_HEADER_GUARD
