// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_cortado_backend_options.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_cortado_backend_options.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Query specification for resolving effective per-output Cortado options. */
    struct output_cortado_backend_options_spec
    {
        using query = output_cortado_backend_options_query;
        using dependencies = rpnx::typelist< output_binary_information_query, target_configuration_query, target_cortado_backend_options_query >;
    };

    /** Resolves target defaults and per-output Cortado option overrides. */
    auto output_cortado_backend_options_impl(std::string input) -> rpnx::querygraph::coroutine< output_cortado_backend_options_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD
