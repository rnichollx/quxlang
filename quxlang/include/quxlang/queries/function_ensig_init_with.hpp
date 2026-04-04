// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_ENSIG_INIT_WITH_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_ENSIG_INIT_WITH_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <optional>


namespace quxlang
{
    struct function_ensig_init_with_query
    {
        static constexpr auto query_id = "function_ensig_init_with";
        using input_type = ensig_initialization;
        using output_type = std::optional< invotype >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_ENSIG_INIT_WITH_HEADER_GUARD
