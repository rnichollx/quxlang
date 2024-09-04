//
// Created by Ryan Nicholl on 4/22/24.
//

#include <quxlang/compiler.hpp>
#include <quxlang/res/instanciation.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(instanciation)
{
    std::string dbg_input = to_string(input_val);
    type_symbol templexoid_symbol = input_val.callee;

    auto kind = co_await QUX_CO_DEP(symbol_type, (templexoid_symbol));

    if (kind == symbol_kind::functum)
    {
        co_return co_await QUX_CO_DEP(functum_instanciation, (input_val));
    }
    else if (kind == symbol_kind::user_function || kind == symbol_kind::builtin_function)
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