// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_DTOR_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_DTOR_SPEC_HEADER_GUARD

#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_default_dtor_spec
    {
        using query = class_default_dtor_query;
        using dependencies = rpnx::typelist< class_type_query, functum_initialize_query, struct_tags_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< class_default_dtor_spec > class_default_dtor_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_DEFAULT_DTOR_SPEC_HEADER_GUARD
