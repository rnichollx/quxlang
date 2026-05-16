// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/function_instanciation_spec.hpp>

#include "quxlang/macros.hpp"
#include <quxlang/queries/function_declaration.hpp>


rpnx::querygraph::coroutine< quxlang::function_instanciation_spec > quxlang::function_instanciation_impl(initialization_reference input)
{
    if (!typeis< temploid_reference >(input.initializee))
    {
        throw quxlang::compiler_bug("Internal Compiler Error(this is a compiler bug): Cannot instanciate a non-function with 'function_instanciation' resolver");
    }

    temploid_reference const& sel_ref = as< temploid_reference >(input.initializee);
    auto selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(sel_ref);

    if (selected_kind == symbol_kind::template_)
    {
        throw quxlang::compiler_bug("function_instanciation received a template selection. Function templates are not directly callable; set template arguments manually before performing function instanciation.");
    }

    if (selected_kind != symbol_kind::function)
    {
        throw quxlang::compiler_bug("Internal Compiler Error(this is a compiler bug): function_instanciation received a temploid selection that is not a function");
    }


    // Get the overload?
    auto formal_ensig = co_await rpnx::querygraph::request< temploid_formal_ensig_query >(sel_ref);
    if (!formal_ensig.has_value())
    {
        throw quxlang::compiler_bug("function_instanciation received an overload reference without a resolvable formal ensig");
    }
    type_symbol type_of_this = void_type{};
    if (typeis< submember >(sel_ref.templexoid))
    {
        type_of_this = as< submember >(sel_ref.templexoid).of;
    }
    auto call_set = co_await rpnx::querygraph::request< function_ensig_init_with_query >(ensig_initialization{
                                                                       .ensig = *formal_ensig,
                                                                       .params = input.parameters,
                                                                       .adaptations = input.adaptations,
                                                                       .type_of_this = type_of_this,
                                                                   });

    if (!call_set)
    {
        QUX_WHY("No overload set found for function instanciation");
        co_return std::nullopt;
    }

    instatype canonical_params = call_set.value();

    auto declaration = co_await rpnx::querygraph::request< function_declaration_query >(sel_ref);
    if (declaration.has_value())
    {
        bool has_explicit_this = false;
        for (auto const& param : declaration->header.call_parameters)
        {
            if (param.api_name == std::optional< std::string >{"THIS"} || param.name == std::optional< std::string >{"THIS"})
            {
                has_explicit_this = true;
                break;
            }
        }

        if (!has_explicit_this)
        {
            auto rewrite_implied_this = [](parameter_instantiation& actual) -> void
            {
                auto rewrite_type = [](type_symbol type) -> std::optional< type_symbol >
                {
                    if (type.template type_is< ptrref_type >())
                    {
                        ptrref_type rewritten = type.template get_as< ptrref_type >();
                        rewritten.target = thistype{};
                        return rewritten;
                    }
                    if (type.template type_is< nvalue_slot >())
                    {
                        nvalue_slot rewritten = type.template get_as< nvalue_slot >();
                        rewritten.target = thistype{};
                        return rewritten;
                    }
                    if (type.template type_is< dvalue_slot >())
                    {
                        dvalue_slot rewritten = type.template get_as< dvalue_slot >();
                        rewritten.target = thistype{};
                        return rewritten;
                    }
                    return std::nullopt;
                };

                auto rewritten = rewrite_type(parameter_instantiation_type(actual));
                if (!rewritten.has_value())
                {
                    return;
                }

                if (actual.template type_is< parameter_value_instantiation >())
                {
                    actual.template get_as< parameter_value_instantiation >().type = *rewritten;
                }
                else
                {
                    actual = make_type_instantiation(*rewritten);
                }
            };

            auto this_it = canonical_params.named.find("THIS");
            if (this_it != canonical_params.named.end())
            {
                rewrite_implied_this(this_it->second);
            }
        }
    }

    auto result = instanciation_reference{.temploid = sel_ref, .params = std::move(canonical_params)};
    co_return result;
}
