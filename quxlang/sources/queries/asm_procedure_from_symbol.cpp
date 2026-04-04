// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp>

#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/macros.hpp>



rpnx::querygraph::coroutine< quxlang::asm_procedure_from_symbol_spec > quxlang::asm_procedure_from_symbol_impl(type_symbol input)
{
    auto ast = co_await rpnx::querygraph::query_request< symboid_query >(input);

    asm_procedure out;

    if (!typeis< ast2_asm_procedure_declaration >(ast))
    {
        throw std::logic_error("Not an asm procedure");
    }

    auto proc = as< ast2_asm_procedure_declaration >(ast);

    if (proc.linkname.has_value())
    {
        out.name = *proc.linkname;
    }

    else
    {
        out.name = mangle(input);
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

                    auto linkname = co_await rpnx::querygraph::query_request< extern_linksymbol_query >(as<ast2_extern>(part));

                    operand_str += linkname;
                }
                else if (typeis< ast2_procedure_ref >(part))
                {
                    auto proc = as< ast2_procedure_ref >(part);

                    contextual_type_reference proc_ctx{
                        .context = input,
                        .type = proc.functanoid,
                        // TODO: calling convention?
                    };

                    auto procedure_canonical = (co_await rpnx::querygraph::query_request< lookup_query >(proc_ctx)).value();
                    auto linkname = co_await rpnx::querygraph::query_request< procedure_linksymbol_query >(ast2_procedure_ref{.cc=proc.cc, .functanoid=procedure_canonical});
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

    co_return out;
}