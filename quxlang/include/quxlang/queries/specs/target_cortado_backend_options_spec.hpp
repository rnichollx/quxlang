// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TARGET_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TARGET_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/target_cortado_backend_options.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Query specification for target-level Cortado backend options. */
    struct target_cortado_backend_options_spec
    {
        using query = target_cortado_backend_options_query;
        using dependencies = rpnx::typelist< target_configuration_query >;
    };

    /** Returns the Cortado options configured on the active target. */
    auto target_cortado_backend_options_impl(std::monostate input) -> rpnx::querygraph::coroutine< target_cortado_backend_options_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TARGET_CORTADO_BACKEND_OPTIONS_SPEC_HEADER_GUARD
