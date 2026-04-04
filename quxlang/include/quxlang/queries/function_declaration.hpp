// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_DECLARATION_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <optional>


namespace quxlang
{
    struct function_declaration_query
    {
        static constexpr auto query_id = "function_declaration";
        using input_type = temploid_reference;
        using output_type = std::optional< ast2_function_declaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_DECLARATION_HEADER_GUARD
