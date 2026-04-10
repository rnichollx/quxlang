// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD

#include <quxlang/queries/class_trivially_destructible.hpp>
#include <quxlang/queries/class_default_dtor.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using class_trivially_destructible_spec = rpnx::querygraph::query_handler_spec< class_trivially_destructible_query, rpnx::typelist< class_default_dtor_query > >;

    rpnx::querygraph::coroutine< class_trivially_destructible_spec > class_trivially_destructible_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CLASS_TRIVIALLY_DESTRUCTIBLE_SPEC_HEADER_GUARD
