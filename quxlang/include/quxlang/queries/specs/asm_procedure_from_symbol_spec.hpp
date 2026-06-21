// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD

#include <quxlang/queries/asm_procedure_from_symbol.hpp>
#include <quxlang/queries/extern_linksymbol.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/procedure_linksymbol.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct asm_procedure_from_symbol_spec
    {
        using query = asm_procedure_from_symbol_query;
        using dependencies = rpnx::typelist< extern_linksymbol_query, instanciation_query, lookup_query, procedure_linksymbol_query, symboid_query, temploid_formal_ensig_query >;
    };

    rpnx::querygraph::coroutine< asm_procedure_from_symbol_spec > asm_procedure_from_symbol_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_ASM_PROCEDURE_FROM_SYMBOL_SPEC_HEADER_GUARD
