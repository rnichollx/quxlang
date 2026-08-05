// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TARGET_CORTADO_BACKEND_OPTIONS_HEADER_GUARD
#define QUXLANG_QUERIES_TARGET_CORTADO_BACKEND_OPTIONS_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>

namespace quxlang
{
    /** Returns Cortado-specific options for the active target. */
    struct target_cortado_backend_options_query
    {
        static constexpr auto query_id = "target_cortado_backend_options";
        using input_type = std::monostate;
        using output_type = backend_cortado_options;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TARGET_CORTADO_BACKEND_OPTIONS_HEADER_GUARD
