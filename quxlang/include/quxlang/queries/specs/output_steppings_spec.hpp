// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_STEPPINGS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_STEPPINGS_SPEC_HEADER_GUARD

#include <quxlang/queries/output_build_settings.hpp>
#include <quxlang/queries/output_steppings.hpp>
#include <quxlang/queries/target_configuration.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** QueryGraph specification for resolving an output's CPU steppings. */
    struct output_steppings_spec
    {
        using query = output_steppings_query;
        using dependencies = rpnx::typelist< target_configuration_query, output_build_settings_query >;
    };

    /** Resolves explicit target steppings or platform defaults selected by the output build type. */
    rpnx::querygraph::coroutine< output_steppings_spec > output_steppings_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_STEPPINGS_SPEC_HEADER_GUARD
