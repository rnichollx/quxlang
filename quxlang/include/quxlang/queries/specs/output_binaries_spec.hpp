// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binaries.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binaries_spec
    {
        using query = output_binaries_query;
        using dependencies = rpnx::typelist<>;
    };

    rpnx::querygraph::coroutine< output_binaries_spec > output_binaries_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_SPEC_HEADER_GUARD
