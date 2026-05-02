// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_BUILTIN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_BUILTIN_SPEC_HEADER_GUARD

#include <quxlang/queries/class_builtin.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_builtin_spec
    {
        using query = class_builtin_query;
        using dependencies = rpnx::typelist<  >;
    };

    rpnx::querygraph::coroutine< class_builtin_spec > class_builtin_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_BUILTIN_SPEC_HEADER_GUARD
