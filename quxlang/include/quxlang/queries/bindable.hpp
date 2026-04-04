// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_BINDABLE_HEADER_GUARD
#define QUXLANG_QUERIES_BINDABLE_HEADER_GUARD

#include <quxlang/data/convertibility_types.hpp>


namespace quxlang
{
    struct bindable_query
    {
        static constexpr auto query_id = "bindable";
        using input_type = implicitly_convertible_to_input;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_BINDABLE_HEADER_GUARD
