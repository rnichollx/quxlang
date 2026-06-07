// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_INFORMATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_INFORMATION_SPEC_HEADER_GUARD

#include <quxlang/queries/output_binaries_information.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_list.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_binaries_information_spec
    {
        using query = output_binaries_information_query;
        using dependencies = rpnx::typelist< output_list_query, output_binary_information_query >;
    };

    rpnx::querygraph::coroutine< output_binaries_information_spec > output_binaries_information_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_BINARIES_INFORMATION_SPEC_HEADER_GUARD
