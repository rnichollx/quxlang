// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>


namespace quxlang
{
    struct functum_list_user_overload_declarations_query
    {
        static constexpr auto query_id = "functum_list_user_overload_declarations";
        using input_type = type_symbol;
        using output_type = std::vector< ast2_function_declaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_LIST_USER_OVERLOAD_DECLARATIONS_HEADER_GUARD
