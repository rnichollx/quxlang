// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL


#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/res/asm_procedure_from_symbol_resolver.hpp>
#include <quxlang/compiler.hpp>


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

    for (auto const& inst : proc.instructions)
    {
        asm_instruction out_inst;
        out_inst.opcode_mnemonic = inst.opcode_mnemonic;

        for (auto const& operand : inst.operands)
        {
            std::string operand_str;
            for (auto const& part : operand.components)
            {
                if (typeis< std::string >(part))
                {
                    operand_str += as< std::string >(part);
                }
                else if (typeis< ast2_extern >(part))
                {
                    auto ext = as< ast2_procedure_ref >(part);

                    QUX_CO_GETDEP(linkname, extern_linksymbol, (as<ast2_extern>(part)));

                    operand_str += linkname;
                }
                else if (typeis< ast2_procedure_ref >(part))
                {
                    auto proc = as< ast2_procedure_ref >(part);

                    contextual_type_reference proc_ctx{
                        .context = input_val,
                        .type = proc.functanoid,
                        // TODO: calling convention?
                    };

                    QUX_CO_GETDEP(procedure_canonical, canonical_symbol_from_contextual_symbol, (proc_ctx));
                    QUX_CO_GETDEP(linkname, procedure_linksymbol, (ast2_procedure_ref{.cc=proc.cc, .functanoid=procedure_canonical}));
                    operand_str += "=" + linkname;
                }
                else
                {
                    rpnx::unimplemented();
                }
            }

            out_inst.operands.push_back(operand_str);

        }

        out.instructions.push_back(out_inst);
    }

    QUX_CO_ANSWER(out);
}