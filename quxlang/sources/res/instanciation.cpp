// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/instanciation.hpp>


#include "quxlang/res/function.hpp"
#include "quxlang/compiler.hpp"


QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_instanciation)
{
    if (!typeis< temploid_reference >(input_val.initializee))
    {
        throw std::logic_error("Internal Compiler Error(this is a compiler bug): Cannot instanciate a non-function with 'function_instanciation' resolver");
    }

    temploid_reference const& sel_ref = as< temploid_reference >(input_val.initializee);


    // Get the overload?
    auto call_set = co_await QUX_CO_DEP(overload_set_instanciate_with, (overload_set_instanciate_with_q{.overload = sel_ref.which, .call = input_val.parameters}));

    if (!call_set)
    {
        QUX_WHY("No overload set found for function instanciation");
        QUX_CO_ANSWER(std::nullopt);
    }

    auto result = initialization_reference{.initializee = input_val.initializee, .parameters = call_set.value()};
    QUX_CO_ANSWER(result);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(instanciation)
{
    std::string dbg_input = to_string(input_val);
    type_symbol templexoid_symbol = input_val.initializee;

    auto kind = co_await QUX_CO_DEP(symbol_type, (templexoid_symbol));

    if (kind == symbol_kind::functum)
    {
        co_return co_await QUX_CO_DEP(functum_instanciation, (input_val));
    }
    else if (kind == symbol_kind::function)
    {
        co_return co_await QUX_CO_DEP(function_instanciation, (input_val));
    }
    else if (kind == symbol_kind::template_)
    {
        co_return co_await QUX_CO_DEP(template_instanciation, (input_val));
    }
    else if (kind == symbol_kind::templex)
    {
       throw rpnx::unimplemented();
       // co_return co_await QUX_CO_DEP(templex_instanciation, (input_val));
    }
    else
    {
        co_return std::nullopt;
      //  throw std::logic_error("Cannot instanciate non-templexoid");
    }
}