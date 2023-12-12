//
// Created by Ryan Nicholl on 11/4/23.
//

#include "rylang/compiler.hpp"

#include "rylang/res/function_qualified_reference_resolver.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/manipulators/qmanip.hpp"

void rylang::function_qualified_reference_resolver::process(compiler* c)
{
   //assert(false);
   type_symbol chain = m_input;

    auto call_overload_set_dp = get_dependency(
        [&]
        {
            return c->lk_function_overload_selection(chain, m_args);
        });

    if (!ready())
        return;

    call_parameter_information cs = call_overload_set_dp->get();

    instanciation_reference psr;
    psr.callee = chain;
    for (auto& argtype : cs.argument_types)
    {
        assert(!qualified_is_contextual(argtype));
        psr.parameters.push_back(argtype);
    }


    set_value(psr);
}
