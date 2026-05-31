// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_OUTPUT_LIST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_OUTPUT_LIST_SPEC_HEADER_GUARD

#include <quxlang/queries/output_list.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct output_list_spec
    {
        using query = output_list_query;
        using dependencies = rpnx::typelist<>;
    };

    rpnx::querygraph::coroutine< output_list_spec > output_list_impl(std::monostate input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_OUTPUT_LIST_SPEC_HEADER_GUARD
