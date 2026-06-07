// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/output_binary_artifacts_spec.hpp>

rpnx::querygraph::coroutine< quxlang::output_binary_artifacts_spec > quxlang::output_binary_artifacts_impl(std::monostate)
{
    co_await rpnx::querygraph::request< run_static_tests_query >(std::monostate{});
    std::map< std::string, output_query_output > const output_information = co_await rpnx::querygraph::request< output_binaries_information_query >(std::monostate{});
    std::map< std::string, output_binary_artifact > output;

    for (std::pair< std::string const, output_query_output > const& output_entry : output_information)
    {
        output.emplace(output_entry.first, co_await rpnx::querygraph::request< output_binary_artifact_query >(output_entry.first));
    }

    co_return output;
}
