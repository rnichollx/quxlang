// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>
#include <quxlang/queries/symboid.hpp>

#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::procedure_linksymbol_spec > quxlang::procedure_linksymbol_impl(ast2_procedure_ref input)
{
    type_symbol asm_declaration_symbol = input.functanoid;
    if (input.functanoid.type_is< instanciation_reference >())
    {
        asm_declaration_symbol = input.functanoid.get_as< instanciation_reference >().temploid.templexoid;
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(asm_declaration_symbol);
    if (typeis< ast2_extern_procedure >(symboid))
    {
        co_return as< ast2_extern_procedure >(symboid).external_symbol_name;
    }

    co_return to_string(input.functanoid);
}
