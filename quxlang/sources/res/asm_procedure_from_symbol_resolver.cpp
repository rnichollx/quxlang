// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL


#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/res/asm_procedure_from_symbol_resolver.hpp>


QUX_CO_RESOLVER_IMPL_FUNC_DEF(asm_procedure_from_symbol)
{
    QUX_CO_GETDEP(ast, entity_ast_from_canonical_chain, (input_val));

    asm_procedure out;

    if (!typeis< ast2_asm_procedure_declaration >(ast))
    {
        throw std::runtime_error("Not an asm procedure");
    }

    auto proc = as< ast2_asm_procedure_declaration >(ast);

    if (proc.linkname.has_value())
    {
        out.name = *proc.linkname;
    }

    else
    {
        out.name = mangle(input_val);
    }

    out.instructions = proc.instructions;

    QUX_CO_ANSWER(out);
}