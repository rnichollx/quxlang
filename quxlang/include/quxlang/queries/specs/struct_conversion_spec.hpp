// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_CONVERSION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_CONVERSION_SPEC_HEADER_GUARD

#include <quxlang/queries/struct_conversion.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_conversion_spec
    {
        using query = struct_conversion_query;
        using dependencies = rpnx::typelist< struct_inheritance_info_query >;
    };

    /** Finds an unambiguous static path from one struct type to a base type. */
    auto struct_conversion_impl(struct_conversion_input input) -> rpnx::querygraph::coroutine< struct_conversion_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_CONVERSION_SPEC_HEADER_GUARD
