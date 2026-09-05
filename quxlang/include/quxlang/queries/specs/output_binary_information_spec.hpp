// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_INFORMATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_INFORMATION_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/target_configuration.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/configured_target.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binary_information_spec
    {
        using query = output_binary_information_query;
        using dependencies = rpnx::typelist< target_configuration_query, source_bundle_query, configured_target_query >;
    };

    rpnx::querygraph::coroutine< output_binary_information_spec > output_binary_information_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_INFORMATION_SPEC_HEADER_GUARD
