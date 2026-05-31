// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binary.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binary_spec
    {
        using query = output_binary_query;
        using dependencies = rpnx::typelist<>;
    };

    rpnx::querygraph::coroutine< output_binary_spec > output_binary_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARY_SPEC_HEADER_GUARD
