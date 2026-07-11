// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_type_spec
    {
        using query = class_type_query;
        using dependencies = rpnx::typelist< class_type_query, subtag_binding_query, symboid_query >;
    };

    /** Classifies the concrete representation family of a constructible type. */
    rpnx::querygraph::coroutine< class_type_spec > class_type_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_TYPE_SPEC_HEADER_GUARD
