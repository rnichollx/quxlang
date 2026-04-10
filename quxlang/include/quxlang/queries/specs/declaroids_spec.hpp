// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_DECLAROIDS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_DECLAROIDS_SPEC_HEADER_GUARD

#include <quxlang/queries/declaroids.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/symboid_subdeclaroids.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using declaroids_spec = rpnx::querygraph::query_handler_spec< declaroids_query, rpnx::typelist< constexpr_bool_query, symboid_subdeclaroids_query > >;

    rpnx::querygraph::coroutine< declaroids_spec > declaroids_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_DECLAROIDS_SPEC_HEADER_GUARD
