// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTIN_OVERLOADS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTIN_OVERLOADS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_builtin_overloads.hpp>
#include <quxlang/queries/functum_builtins.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using functum_builtin_overloads_spec = rpnx::query_handler_spec< functum_builtin_overloads_query, rpnx::typelist< functum_builtins_query > >;

    rpnx::querygraph::coroutine< functum_builtin_overloads_spec > functum_builtin_overloads_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTIN_OVERLOADS_SPEC_HEADER_GUARD
