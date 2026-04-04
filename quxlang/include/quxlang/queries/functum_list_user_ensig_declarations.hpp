// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_HEADER_GUARD

#include <quxlang/data/tempar_types.hpp>
#include <quxlang/data/type_symbol.hpp>
#include <vector>


namespace quxlang
{
    struct functum_list_user_ensig_declarations_query
    {
        static constexpr auto query_id = "functum_list_user_ensig_declarations";
        using input_type = type_symbol;
        using output_type = std::vector< temploid_ensig >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_HEADER_GUARD
