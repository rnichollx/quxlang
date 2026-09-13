// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_PUBLISHED_NAME_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_PUBLISHED_NAME_INFO_HEADER_GUARD
#include <quxlang/data/body_types.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
namespace quxlang
{
    /// Names introduced or updated at one immutable body level.
    struct published_name_info
    {
        static constexpr auto subquery_id = "published_name_info";
        using parent_query = vm_procedure3_query;
        using input_type = std::uint64_t;
        using output_type = std::map< std::string, published_name >;
    };
}
#endif
