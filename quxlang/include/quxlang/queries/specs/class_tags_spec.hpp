// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_TAGS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_TAGS_SPEC_HEADER_GUARD

#include <quxlang/queries/class_tags.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct class_tags_spec
    {
        using query = class_tags_query;
        using dependencies = rpnx::typelist< symboid_query >;
    };

    rpnx::querygraph::coroutine< class_tags_spec > class_tags_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_TAGS_SPEC_HEADER_GUARD
