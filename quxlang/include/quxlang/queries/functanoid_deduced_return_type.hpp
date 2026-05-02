// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_DEDUCED_RETURN_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_DEDUCED_RETURN_TYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <variant>

namespace quxlang
{
    struct functanoid_deduced_return_type
    {
        static constexpr auto subquery_id = "functanoid_deduced_return_type";
        using parent_query = user_vm_procedure3_query;
        using input_type = std::monostate;
        using output_type = type_symbol;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_DEDUCED_RETURN_TYPE_HEADER_GUARD
