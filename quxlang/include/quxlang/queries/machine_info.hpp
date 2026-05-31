// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MACHINE_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_MACHINE_INFO_HEADER_GUARD

#include <quxlang/data/machine.hpp>

#include <variant>

namespace quxlang
{
    struct machine_info_query
    {
        static constexpr auto query_id = "machine_info";
        using input_type = std::monostate;
        using output_type = machine_target_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MACHINE_INFO_HEADER_GUARD
