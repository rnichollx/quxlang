// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_EXISTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_EXISTS_SPEC_HEADER_GUARD

#include <quxlang/queries/exists.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using exists_spec = rpnx::query_handler_spec< exists_query, rpnx::typelist< symbol_type_query > >;

    rpnx::querygraph::coroutine< exists_spec > exists_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_EXISTS_SPEC_HEADER_GUARD
