// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_instanciation_spec.hpp>




rpnx::querygraph::coroutine< quxlang::function_instanciation_spec > quxlang::function_instanciation_impl(initialization_reference input)
{
    if (!typeis< temploid_reference >(input.initializee))
    {
        throw std::logic_error("Internal Compiler Error(this is a compiler bug): Cannot instanciate a non-function with 'function_instanciation' resolver");
    }

    temploid_reference const& sel_ref = as< temploid_reference >(input.initializee);
    auto selected_kind = co_await rpnx::querygraph::request< symbol_type_query >(sel_ref);

    if (selected_kind == symbol_kind::template_)
    {
        throw std::logic_error("function_instanciation received a template selection. Function templates are not directly callable; set template arguments manually before performing function instanciation.");
    }

    if (selected_kind != symbol_kind::function)
    {
        throw std::logic_error("Internal Compiler Error(this is a compiler bug): function_instanciation received a temploid selection that is not a function");
    }


    // Get the overload?
    auto call_set = co_await rpnx::querygraph::request< function_ensig_init_with_query >(ensig_initialization{
                                                                       .ensig = sel_ref.which,
                                                                       .params = input.parameters,
                                                                       .adaptations = input.adaptations,
                                                                   });

    if (!call_set)
    {
        QUX_WHY("No overload set found for function instanciation");
        co_return std::nullopt;
    }

    auto result = instanciation_reference{.temploid = sel_ref, .params=call_set.value()};
    co_return result;
}
