// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/symboid_subdeclaroids_spec.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/queries/lookup.hpp>

#include <deque>
#include <set>
// This file implements the symboid_sub_declaroids resolver.



rpnx::querygraph::coroutine< quxlang::symboid_subdeclaroids_spec > quxlang::symboid_subdeclaroids_impl(type_symbol input)
{
    auto sym = co_await rpnx::querygraph::request< symboid_query >(input);

    if (typeis< ast2_struct_declaration >(sym))
    {
        co_return as< ast2_struct_declaration >(sym).declarations;
    }
    else if (typeis< ast2_union_declaration >(sym))
    {
        co_return as< ast2_union_declaration >(sym).declarations;
    }
    else if (typeis< ast2_variant_declaration >(sym))
    {
        co_return as< ast2_variant_declaration >(sym).declarations;
    }
    else if (typeis< ast2_generic_declaration >(sym))
    {
        ast2_generic_declaration const& generic = as< ast2_generic_declaration >(sym);
        ast2_interface_declaration erased_interface;
        std::vector< ast2_interface_function_declaration > generic_functions = generic.functions;

        /** One unresolved edge in the transitive generic contract graph. */
        struct generic_implements_entry
        {
            type_symbol context;
            type_symbol declared_type;
            std::set< type_symbol > ancestry;
        };

        std::deque< generic_implements_entry > pending_implements;
        for (type_symbol const& declared_type : generic.implemented_generics)
        {
            pending_implements.push_back(generic_implements_entry{
                .context = type_parent(input).value_or(type_symbol(void_type{})),
                .declared_type = declared_type,
                .ancestry = {input},
            });
        }
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
            ast2_symboid implemented_symboid = co_await rpnx::querygraph::request< symboid_query >(*resolved_type);
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

            generic_functions.insert(generic_functions.end(), implemented_generic.functions.begin(), implemented_generic.functions.end());
            type_symbol implemented_context = type_parent(*resolved_type).value_or(type_symbol(void_type{}));
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

        for (ast2_interface_function_declaration const& function : generic_functions)
        {
            ast2_interface_function_declaration erased_function = function;
            std::size_t this_parameter_count = 0;
            for (ast2_function_parameter& parameter : erased_function.header.call_parameters)
            {
                if (parameter.api_name != std::optional< std::string >{"THIS"} && parameter.name != std::optional< std::string >{"THIS"})
                {
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
            erased_function.has_default_body = false;
            erased_function.definition.body = {};
            erased_interface.functions.push_back(std::move(erased_function));
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

        std::vector< subdeclaroid > output;
        output.push_back(global_subdeclaroid{
            .decl = std::move(erased_interface),
            .name = "__INTERFACE",
        });
        output.push_back(member_subdeclaroid{
            .decl = ast2_variable_declaration{.type = subsymbol{.of = input, .name = "__INTERFACE"}},
            .name = "__INTERFACE_VAL",
        });
        output.push_back(member_subdeclaroid{
            .decl = ast2_variable_declaration{.type = ptrref_type{
                                                  .target = void_type{},
                                                  .ptr_class = pointer_class::instance,
                                                  .qual = generic.is_const ? qualifier::constant : qualifier::mut,
                                              }},
            .name = "__VALUE",
        });
        co_return output;
    }
    else if (typeis< ast2_implementation_declaration >(sym))
    {
        co_return as< ast2_implementation_declaration >(sym).declarations;
    }
    else if (typeis< ast2_enum_declaration >(sym))
    {
        co_return as< ast2_enum_declaration >(sym).declarations;
    }
    else if (typeis< ast2_flagset_declaration >(sym))
    {
        co_return as< ast2_flagset_declaration >(sym).declarations;
    }
    else if (typeis< ast2_module_declaration >(sym))
    {
        co_return as< ast2_module_declaration >(sym).declarations;
    }
    else if (typeis< ast2_namespace_declaration >(sym))
    {
        co_return as< ast2_namespace_declaration >(sym).declarations;
    }
    else if (typeis< ast2_template_declaration >(sym))
    {
        // Templates don't have subdeclaroids, only a template instanciation could,
        // but that would produce a class, not a template.
        // e.g. ::foo#(I32) would produce a class, not a template.
        // whereas ::foo doesn't have any subdeclaroids, even if ::foo#(I32) does.
        co_return {};
    }
    else if (typeis<functum>(sym))
    {
        co_return {};
    }
    else
    {
       co_return {};
    }
}
