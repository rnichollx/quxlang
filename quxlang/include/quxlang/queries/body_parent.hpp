// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_BODY_PARENT_HEADER_GUARD
#define QUXLANG_QUERIES_BODY_PARENT_HEADER_GUARD
#include <quxlang/queries/vm_procedure3.hpp>
namespace quxlang
{
    /// Explicit lexical predecessor of a body within its VM procedure.
    struct body_parent
    {
        static constexpr auto subquery_id = "body_parent";
        using parent_query = vm_procedure3_query;
        using input_type = std::uint64_t;
        using output_type = std::optional< std::uint64_t >;
    };
}
#endif
