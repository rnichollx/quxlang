// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LIST_USER_FUNCTUM_FORMAL_PARATYPES_HEADER_GUARD
#define QUXLANG_QUERIES_LIST_USER_FUNCTUM_FORMAL_PARATYPES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <vector>


namespace quxlang
{
    struct list_user_functum_formal_paratypes_query
    {
        static constexpr auto query_id = "list_user_functum_formal_paratypes";
        using input_type = type_symbol;
        using output_type = std::vector< paratype >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LIST_USER_FUNCTUM_FORMAL_PARATYPES_HEADER_GUARD
