// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp>

#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/macros.hpp>



rpnx::querygraph::coroutine< quxlang::asm_procedure_from_symbol_spec > quxlang::asm_procedure_from_symbol_impl(type_symbol input)
{
    type_symbol declaration_symbol = input;
    std::optional< std::uint64_t > selected_overload_id;
    if (typeis< instanciation_reference >(input))
    {
        selected_overload_id = as< instanciation_reference >(input).temploid.overload_id;
        declaration_symbol = as< instanciation_reference >(input).temploid.templexoid;
    }
    else if (typeis< temploid_reference >(input))
    {
        selected_overload_id = as< temploid_reference >(input).overload_id;
        declaration_symbol = as< temploid_reference >(input).templexoid;
    }

    auto ast = co_await rpnx::querygraph::request< symboid_query >(declaration_symbol);

    asm_procedure out;

    if (!typeis< ast2_asm_procedure_declaration >(ast))
    {
        throw quxlang::compiler_bug("Not an asm procedure");
    }

    auto proc = as< ast2_asm_procedure_declaration >(ast);
    out.architecture = proc.architecture;

    out.name = mangle(input);

    if (!proc.callable_interfaces.empty())
    {
        std::uint64_t callable_index = selected_overload_id.value_or(0);
        if (!selected_overload_id.has_value() && proc.callable_interfaces.size() != 1)
        {
            throw quxlang::compiler_bug("Callable asm procedure requires an explicit overload selection");
        }
        if (callable_index >= proc.callable_interfaces.size())
        {
            throw quxlang::compiler_bug("Asm procedure overload id is out of range");
        }

        ast2_asm_callable const& callable = proc.callable_interfaces.at(static_cast< std::size_t >(callable_index));
        asm_callable selected_callable;
        selected_callable.calling_conv = callable.calling_conv;
        selected_callable.clobber = callable.clobber;
        selected_callable.return_register_name = callable.return_register_name;
        selected_callable.return_type = callable.return_type;
        for (ast2_argument_interface const& argument : callable.args)
        {
            selected_callable.args.push_back(asm_argument_binding{
                .register_name = argument.register_name,
                .type = argument.type,
            });
        }
        out.callable_interface = std::move(selected_callable);
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

                    auto linkname = co_await rpnx::querygraph::request< extern_linksymbol_query >(as<ast2_extern>(part));

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

                    auto procedure_canonical = (co_await rpnx::querygraph::request< lookup_query >(proc_ctx)).value();
                    auto linkname = co_await rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc=proc.cc, .functanoid=procedure_canonical});
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
