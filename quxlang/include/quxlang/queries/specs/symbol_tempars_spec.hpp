// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD

#include <quxlang/queries/symbol_tempars.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct symbol_tempars_spec
    {
        using query = symbol_tempars_query;
        using dependencies = rpnx::typelist< temploid_formal_ensig_query >;
    };

    rpnx::querygraph::coroutine< symbol_tempars_spec > symbol_tempars_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_SYMBOL_TEMPARS_SPEC_HEADER_GUARD
