// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SNAPSHOT_VALUE_HEADER_GUARD
#define QUXLANG_QUERIES_SNAPSHOT_VALUE_HEADER_GUARD

#include <quxlang/queries/vm_procedure3.hpp>

namespace quxlang
{
    /// Frozen serialized initializer of one generated static object.
    struct snapshot_value
    {
        type_symbol type;
        constexpr_serialoid value;
        RPNX_MEMBER_METADATA(snapshot_value, type, value);
    };

    /// Publishes a generated snapshot before its owning procedure finishes generation.
    struct snapshot_value_subquery
    {
        static constexpr auto subquery_id = "snapshot_value";
        using parent_query = vm_procedure3_query;
        using input_type = std::uint64_t;
        using output_type = snapshot_value;
    };
}
#endif
