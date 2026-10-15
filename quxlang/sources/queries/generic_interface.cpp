// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/generic_interface_spec.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/data/generic_contract.hpp>
#include <quxlang/manipulators/typeutils.hpp>

#include <deque>
#include <set>

rpnx::querygraph::coroutine< quxlang::generic_interface_spec > quxlang::generic_interface_impl(type_symbol input)
{
    ast2_symboid const& declaration = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!declaration.type_is< ast2_generic_declaration >())
    {
        throw compiler_bug("Cannot synthesize a generic interface for a non-generic: " + to_string(input));
    }

    ast2_generic_declaration const& generic = as< ast2_generic_declaration >(declaration);
    ast2_interface_declaration erased_interface;

    std::deque< generic_implements_entry > pending_implements{
        generic_implements_entry{
            .context = type_parent(input).value_or(type_symbol(void_type{})),
            .declared_type = input,
            .ancestry = {},
        },
    };
    std::set< type_symbol > expanded_generics;

    while (!pending_implements.empty())
    {
        generic_implements_entry implements_entry = std::move(pending_implements.front());
        pending_implements.pop_front();
        std::string declared_type_name = to_string(implements_entry.declared_type);
        std::optional< type_symbol > resolved_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
            .context = std::move(implements_entry.context),
            .type = std::move(implements_entry.declared_type),
        });
        if (!resolved_type.has_value())
        {
            throw semantic_compilation_error("Generic IMPLEMENTS target could not be resolved: " + declared_type_name);
        }
        ast2_symboid const& implemented_symboid = co_await rpnx::querygraph::request< symboid_query >(*resolved_type);
        if (!implemented_symboid.type_is< ast2_generic_declaration >())
        {
            throw semantic_compilation_error("Generic IMPLEMENTS target is not a generic: " + to_string(*resolved_type));
        }
        ast2_generic_declaration const& implemented_generic = implemented_symboid.get_as< ast2_generic_declaration >();
        if (generic.is_reference && !implemented_generic.is_reference)
        {
            throw semantic_compilation_error("GENERIC_REF cannot implement owning GENERIC: " + to_string(*resolved_type));
        }
        if (!generic.copyable && implemented_generic.copyable)
        {
            throw semantic_compilation_error("MOVE_ONLY generic cannot implement a copyable generic: " + to_string(*resolved_type));
        }
        if (!generic.comparable && implemented_generic.comparable)
        {
            throw semantic_compilation_error("INCOMPARABLE generic cannot implement a comparable generic: " + to_string(*resolved_type));
        }
        if (implements_entry.ancestry.contains(*resolved_type))
        {
            throw semantic_compilation_error("Cyclic generic IMPLEMENTS target: " + to_string(*resolved_type));
        }
        if (!expanded_generics.insert(*resolved_type).second)
        {
            continue;
        }

        type_symbol implemented_context = type_parent(*resolved_type).value_or(type_symbol(void_type{}));
        for (ast2_interface_function_declaration const& function : implemented_generic.functions)
        {
            ast2_interface_function_declaration erased_function = function;
            std::size_t this_parameter_count = 0;
            for (ast2_function_parameter& parameter : erased_function.header.call_parameters)
            {
                if (parameter.api_name != std::optional< std::string >{"THIS"} && parameter.name != std::optional< std::string >{"THIS"})
                {
                    std::optional< type_symbol > resolved_parameter = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                        .context = implemented_context,
                        .type = parameter.type,
                    });
                    if (!resolved_parameter.has_value())
                    {
                        throw semantic_compilation_error("Generic function parameter type could not be resolved");
                    }
                    parameter.type = std::move(*resolved_parameter);
                    continue;
                }

                ++this_parameter_count;
                if (!parameter.type.type_is< ptrref_type >())
                {
                    throw semantic_compilation_error("Generic function THIS must be a reference to THISTYPE");
                }
                ptrref_type const& this_type = parameter.type.get_as< ptrref_type >();
                if (this_type.ptr_class != pointer_class::ref || !this_type.target.type_is< thistype >())
                {
                    throw semantic_compilation_error("Generic function THIS must be a reference to THISTYPE");
                }
                if (generic.is_const && this_type.qual != qualifier::constant)
                {
                    throw semantic_compilation_error("CONST GENERIC_REF functions must accept CONST& THISTYPE");
                }

                parameter.api_name = "GENERIC_THIS";
                parameter.name = "GENERIC_THIS";
                parameter.type = ptrref_type{
                    .target = void_type{},
                    .ptr_class = pointer_class::instance,
                    .qual = this_type.qual == qualifier::constant ? qualifier::constant : qualifier::mut,
                };
            }
            if (this_parameter_count != 1)
            {
                throw semantic_compilation_error("Generic functions must declare exactly one THIS parameter");
            }
            if (erased_function.definition.return_type.has_value())
            {
                std::optional< type_symbol > resolved_return_type = co_await rpnx::querygraph::request< lookup_query >(contextual_type_reference{
                    .context = implemented_context,
                    .type = *erased_function.definition.return_type,
                });
                if (!resolved_return_type.has_value())
                {
                    throw semantic_compilation_error("Generic function return type could not be resolved");
                }
                erased_function.definition.return_type = std::move(*resolved_return_type);
            }
            erased_function.has_default_body = false;
            erased_function.definition.body = {};
            erased_interface.functions.push_back(std::move(erased_function));
        }

        implements_entry.ancestry.insert(*resolved_type);
        for (type_symbol const& inherited_type : implemented_generic.implemented_generics)
        {
            pending_implements.push_back(generic_implements_entry{
                .context = implemented_context,
                .declared_type = inherited_type,
                .ancestry = implements_entry.ancestry,
            });
        }
    }

    auto append_lifecycle_function = [&](std::string name, std::vector< ast2_function_parameter > parameters, type_symbol return_type)
    {
        ast2_interface_function_declaration function;
        function.name = std::move(name);
        function.header.call_parameters = std::move(parameters);
        function.definition.return_type = std::move(return_type);
        erased_interface.functions.push_back(std::move(function));
    };
    type_symbol mutable_erased_pointer = ptrref_type{.target = void_type{}, .ptr_class = pointer_class::instance, .qual = qualifier::mut};
    type_symbol constant_erased_pointer = ptrref_type{.target = void_type{}, .ptr_class = pointer_class::instance, .qual = qualifier::constant};
    append_lifecycle_function("__CURRENT_TYPE", {}, type_index_type{});
    if (!generic.is_reference)
    {
        append_lifecycle_function("__DELETE", {ast2_function_parameter{.name = "GENERIC_THIS", .api_name = "GENERIC_THIS", .type = mutable_erased_pointer}}, void_type{});
        if (generic.copyable)
        {
            append_lifecycle_function("__COPY", {ast2_function_parameter{.name = "GENERIC_THIS", .api_name = "GENERIC_THIS", .type = constant_erased_pointer}}, mutable_erased_pointer);
        }
    }
    if (generic.comparable)
    {
        std::vector< ast2_function_parameter > comparison_parameters{
            ast2_function_parameter{.name = "GENERIC_THIS", .api_name = "GENERIC_THIS", .type = constant_erased_pointer},
            ast2_function_parameter{.name = "OTHER", .api_name = "OTHER", .type = constant_erased_pointer},
        };
        append_lifecycle_function("__COMPARE", comparison_parameters, builtin_symbol{"ORDER"});
        append_lifecycle_function("__COMPARE_EQ", std::move(comparison_parameters), bool_type{});
    }

    co_return erased_interface;
}
