// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/asm_procedure_from_symbol_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

#include <quxlang/asm/asm.hpp>
#include <quxlang/backends/asm/symbol_format.hpp>
#include <quxlang/macros.hpp>
#include <rpnx/annex.hpp>

#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct asm_procedure_from_symbol_helpers
    {
        /// Resolves one ASM CALLABLE surface type in the declaring procedure's lexical context.
        static auto resolve_asm_callable_type(type_symbol const& context, type_symbol type)
            -> rpnx::querygraph::cosubroutine_impl< asm_procedure_from_symbol_spec, type_symbol >
        {
            if (type.type_is< ptrref_type >())
            {
                ptrref_type ref = type.get_as< ptrref_type >();
                ref.target = co_await resolve_asm_callable_type(context, std::move(ref.target));
                type = std::move(ref);
            }
            else if (type.type_is< array_type >())
            {
                array_type array = type.get_as< array_type >();
                array.element_type = co_await resolve_asm_callable_type(context, std::move(array.element_type));
                type = std::move(array);
            }
            else if (type.type_is< attached_type_reference >())
            {
                attached_type_reference attached = type.get_as< attached_type_reference >();
                if (!attached.carrying_type.type_is< void_type >())
                {
                    attached.carrying_type = co_await resolve_asm_callable_type(context, std::move(attached.carrying_type));
                }
                type = std::move(attached);
            }
            else if (type.type_is< storage >())
            {
                storage storage_type;
                for (type_symbol const& storable_type : type.get_as< storage >().storable_types)
                {
                    storage_type.storable_types.insert(co_await resolve_asm_callable_type(context, storable_type));
                }
                co_return storage_type;
            }
            else if (type.type_is< procedure_type >())
            {
                procedure_type procedure = type.get_as< procedure_type >();
                for (type_symbol& positional : procedure.signature.params.positional)
                {
                    positional = co_await resolve_asm_callable_type(context, std::move(positional));
                }
                for (std::pair< std::string const, type_symbol >& named : procedure.signature.params.named)
                {
                    named.second = co_await resolve_asm_callable_type(context, std::move(named.second));
                }
                if (procedure.signature.return_type.has_value())
                {
                    procedure.signature.return_type = co_await resolve_asm_callable_type(context, std::move(*procedure.signature.return_type));
                }
                type = std::move(procedure);
            }

            std::optional< type_symbol > const resolved_type =
                co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = context,
                    .type = std::move(type),
                });
            if (!resolved_type.has_value())
            {
                throw semantic_compilation_error("ASM CALLABLE type could not be resolved in declaration context: " + to_string(context));
            }
            co_return *resolved_type;
        }
    };
} // namespace quxlang::detail


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

    auto const& ast = co_await rpnx::querygraph::request< symboid_query >(declaration_symbol);

    rpnx::annex<asm_procedure> out = asm_procedure{};


    if (typeis< ast2_extern_procedure >(ast))
    {
        auto proc = as< ast2_extern_procedure >(ast);
        out->architecture = "";
        out->name = to_string(declaration_symbol);

        if (proc.callable.has_value())
        {
            ast2_asm_callable const& callable = *proc.callable;
            asm_callable selected_callable;
            selected_callable.calling_conv = callable.calling_conv;
            selected_callable.clobber = callable.clobber;
            selected_callable.return_register_name = callable.return_register_name;
            if (callable.return_type.has_value())
            {
                selected_callable.return_type = co_await detail::asm_procedure_from_symbol_helpers::resolve_asm_callable_type(declaration_symbol, *callable.return_type);
            }
            for (ast2_argument_interface const& argument : callable.args)
            {
                // workaround: without this temporary, msvc crashes when trying to compile this code
                // has something to do with co-await inside an aggregate intializer
                type_symbol argument_type =
                    co_await detail::asm_procedure_from_symbol_helpers::resolve_asm_callable_type(declaration_symbol, argument.type);
                selected_callable.args.push_back(asm_argument_binding{
                    .api_name = argument.api_name,
                    .register_name = argument.register_name,
                    .type = std::move(argument_type),
                });
            }
            out->callable_interface = std::move(selected_callable);
        }
        co_return *out;
    }

    if (!typeis< ast2_asm_procedure_declaration >(ast))
    {
        throw quxlang::compiler_bug("Not an asm procedure");
    }

    auto proc = as< ast2_asm_procedure_declaration >(ast);
    if (proc.kind == ast2_asm_declaration_kind::inline_function)
    {
        throw quxlang::semantic_compilation_error("ASM_INLINE_FUNCTION backend support is not implemented");
    }

    out->architecture = proc.architecture;

    out->name = to_string(declaration_symbol);

    if (!proc.callable_interfaces.empty() && (selected_overload_id.has_value() || proc.callable_interfaces.size() == 1))
    {
        std::uint64_t callable_index = selected_overload_id.value_or(0);
        if (callable_index >= proc.callable_interfaces.size())
        {
            throw quxlang::compiler_bug("Asm procedure overload id is out of range");
        }

        ast2_asm_callable const& callable = proc.callable_interfaces.at(static_cast< std::size_t >(callable_index));
        asm_callable selected_callable;
        selected_callable.calling_conv = callable.calling_conv;
        selected_callable.clobber = callable.clobber;
        selected_callable.return_register_name = callable.return_register_name;
        if (callable.return_type.has_value())
        {
            selected_callable.return_type = co_await detail::asm_procedure_from_symbol_helpers::resolve_asm_callable_type(declaration_symbol, *callable.return_type);
        }
        for (ast2_argument_interface const& argument : callable.args)
        {
            type_symbol argument_type =
                co_await detail::asm_procedure_from_symbol_helpers::resolve_asm_callable_type(declaration_symbol, argument.type);
            selected_callable.args.push_back(asm_argument_binding{
                .api_name = argument.api_name,
                .register_name = argument.register_name,
                .type = std::move(argument_type),
            });
        }
        out->callable_interface = std::move(selected_callable);
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
                    ast2_procedure_ref const procedure_ref = as< ast2_procedure_ref >(part);

                    contextual_type_reference proc_ctx{
                        .context = input,
                        .type = procedure_ref.functanoid,
                        // TODO: calling convention?
                    };

                    type_symbol procedure_canonical = (co_await rpnx::querygraph::request< lookup_query >(proc_ctx)).value();
                    if (typeis< temploid_reference >(procedure_canonical))
                    {
                        temploid_reference const& selection = as< temploid_reference >(procedure_canonical);
                        std::optional< temploid_ensig > const formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(selection);
                        if (!formal_ensig.has_value())
                        {
                            throw quxlang::semantic_compilation_error("Cannot resolve selected PROCEDURE_REF overload: " + quxlang::to_string(procedure_ref.functanoid));
                        }
                        if (overload_has_unspecialized_parameters(*formal_ensig))
                        {
                            throw quxlang::semantic_compilation_error("Cannot emit uninstantiated PROCEDURE_REF target: " + quxlang::to_string(procedure_ref.functanoid));
                        }
                        procedure_canonical = instanciation_reference{
                            .temploid = selection,
                            .params = instantiate_declared_overload(*formal_ensig),
                        };
                    }
                    else if (typeis< initialization_reference >(procedure_canonical))
                    {
                        std::optional< instanciation_reference > const instanciation = co_await rpnx::querygraph::request< instanciation_query >(as< initialization_reference >(procedure_canonical));
                        if (!instanciation.has_value())
                        {
                            throw quxlang::semantic_compilation_error("PROCEDURE_REF target is not callable as a concrete function: " + quxlang::to_string(procedure_ref.functanoid));
                        }
                        procedure_canonical = *instanciation;
                    }

                    std::string const linkname =
                        co_await rpnx::querygraph::request< procedure_linksymbol_query >(ast2_procedure_ref{.cc = procedure_ref.cc, .functanoid = procedure_canonical});
                    std::string const formatted_symbol = format_asm_symbol_name(linkname);
                    bool const is_arm_procedure = proc.architecture == "ARM32" || proc.architecture == "ARM64";
                    operand_str += is_arm_procedure ? "=" + formatted_symbol : formatted_symbol;
                }
                else if (typeis< ast2_object_ref >(part))
                {
                    auto object_ref = as< ast2_object_ref >(part);

                    contextual_type_reference object_ctx{
                        .context = input,
                        .type = object_ref.object,
                    };

                    auto object_canonical = co_await rpnx::querygraph::request< lookup_query >(object_ctx);
                    if (!object_canonical.has_value())
                    {
                        throw quxlang::semantic_compilation_error("OBJECT_REF target could not be resolved: " + quxlang::to_string(object_ref.object));
                    }
                    operand_str += quxlang::format_asm_symbol_name(quxlang::to_string(*object_canonical));
                }
                else
                {
                    rpnx::unimplemented();
                }
            }

            out_inst.operands.push_back(operand_str);

        }

        out->instructions.push_back(out_inst);
    }

    co_return *out;
}
