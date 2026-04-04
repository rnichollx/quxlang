// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_MAP_USER_FORMAL_ENSIGS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_MAP_USER_FORMAL_ENSIGS_HEADER_GUARD

#include <quxlang/data/functum_types.hpp>
#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct functum_map_user_formal_ensigs_query
    {
        static constexpr auto query_id = "functum_map_user_formal_ensigs";
        using input_type = type_symbol;
        using output_type = functum_map_formal_ensigs_output_type;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_MAP_USER_FORMAL_ENSIGS_HEADER_GUARD
