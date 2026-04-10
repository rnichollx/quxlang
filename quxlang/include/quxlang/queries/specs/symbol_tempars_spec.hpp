// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD

#include <quxlang/queries/symbol_tempars.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using symbol_tempars_spec = rpnx::querygraph::query_handler_spec< symbol_tempars_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< symbol_tempars_spec > symbol_tempars_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD
