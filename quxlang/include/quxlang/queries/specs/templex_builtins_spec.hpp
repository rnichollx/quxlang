// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTINS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTINS_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/templex_builtins.hpp>

namespace quxlang
{
    using templex_builtins_spec = rpnx::querygraph::query_handler_spec< templex_builtins_query, rpnx::typelist< > >;

    rpnx::querygraph::coroutine< templex_builtins_spec > templex_builtins_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_TEMPLEX_BUILTINS_SPEC_HEADER_GUARD
