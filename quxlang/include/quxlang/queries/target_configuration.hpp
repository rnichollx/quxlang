// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TARGET_CONFIGURATION_HEADER_GUARD
#define QUXLANG_QUERIES_TARGET_CONFIGURATION_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>

namespace quxlang
{
    struct target_configuration_query
    {
        static constexpr auto query_id = "target_configuration";
        using input_type = std::monostate;
        using output_type = target_configuration;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TARGET_CONFIGURATION_HEADER_GUARD
