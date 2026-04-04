// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ENSIG_TEMPARS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ENSIG_TEMPARS_SPEC_HEADER_GUARD

#include <quxlang/queries/ensig_tempars.hpp>
#include <quxlang/queries/symbol_tempars.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using ensig_tempars_spec = rpnx::query_handler_spec< ensig_tempars_query, rpnx::typelist< symbol_tempars_query > >;

    rpnx::querygraph::coroutine< ensig_tempars_spec > ensig_tempars_impl(temploid_ensig input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ENSIG_TEMPARS_SPEC_HEADER_GUARD
