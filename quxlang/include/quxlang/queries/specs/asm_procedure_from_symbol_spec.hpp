// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/asm_procedure_from_symbol.hpp>
#include <quxlang/queries/extern_linksymbol.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/procedure_linksymbol.hpp>
#include <quxlang/queries/symboid.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using asm_procedure_from_symbol_spec = rpnx::querygraph::query_handler_spec< asm_procedure_from_symbol_query, rpnx::typelist< extern_linksymbol_query, lookup_query, procedure_linksymbol_query, symboid_query > >;

    rpnx::querygraph::coroutine< asm_procedure_from_symbol_spec > asm_procedure_from_symbol_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD
