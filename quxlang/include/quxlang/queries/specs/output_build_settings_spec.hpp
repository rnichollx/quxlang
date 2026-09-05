// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BUILD_SETTINGS_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BUILD_SETTINGS_HEADER_GUARD
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_build_settings.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <rpnx/querygraph/querygraph.hpp>
namespace quxlang
{
    /** Dependencies used to resolve explicit and inherited build policies. */
    struct output_build_settings_spec
    {
        using query = output_build_settings_query;
        using dependencies = rpnx::typelist< output_binary_information_query, source_bundle_query, target_configuration_query >;
    };
    /** Validates ambiguity before returning concrete build policies. */
    rpnx::querygraph::coroutine< output_build_settings_spec > output_build_settings_impl(std::string input);
} // namespace quxlang
#endif
