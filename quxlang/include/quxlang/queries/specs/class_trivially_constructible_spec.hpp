// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_CONSTRUCTIBLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_CONSTRUCTIBLE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_trivially_constructible.hpp>
#include <quxlang/queries/class_default_ctor.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_trivially_constructible_spec
    {
        using query = class_trivially_constructible_query;
        using dependencies = rpnx::typelist< class_default_ctor_query >;
    };

    rpnx::querygraph::coroutine< class_trivially_constructible_spec > class_trivially_constructible_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_CONSTRUCTIBLE_SPEC_HEADER_GUARD
