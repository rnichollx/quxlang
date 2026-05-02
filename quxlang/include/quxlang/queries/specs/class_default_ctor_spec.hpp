// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_CTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_CTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_default_ctor.hpp>
#include <quxlang/queries/functum_initialize.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_default_ctor_spec
    {
        using query = class_default_ctor_query;
        using dependencies = rpnx::typelist< functum_initialize_query >;
    };

    rpnx::querygraph::coroutine< class_default_ctor_spec > class_default_ctor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_CTOR_SPEC_HEADER_GUARD
