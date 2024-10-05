// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/function_instanciation.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_instanciation)
{
    if (!typeis< selection_reference >(input_val.callee))
    {
        throw std::logic_error("Internal Compiler Error(this is a compiler bug): Cannot instanciate a non-function with 'function_instanciation' resolver");
    }

    selection_reference const& sel_ref = as< selection_reference >(input_val.callee);


    // Get the overload?
    auto call_set = co_await QUX_CO_DEP(overload_set_instanciate_with, (overload_set_instanciate_with_q{.overload = sel_ref.overload, .call = input_val.parameters}));

    if (!call_set)
    {
        QUX_WHY("No overload set found for function instanciation");
        QUX_CO_ANSWER(std::nullopt);
    }

    auto result = instanciation_reference{.callee = input_val.callee, .parameters = call_set.value()};
    QUX_CO_ANSWER(result);
}