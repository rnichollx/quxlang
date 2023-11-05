//
// Created by Ryan Nicholl on 11/4/23.
//
#include "rylang/res/function_qualified_reference_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/manipulators/qualified_reference.hpp"

void rylang::function_qualified_reference_resolver::process(compiler* c)
{
    // TODO: Lookup chain is deprecated, but this is being used by other resolvers still
    qualified_symbol_reference chain = m_input;

    auto call_overload_set_dp = get_dependency(
        [&]
        {
            return c->lk_function_overload_selection(chain, m_args);
        });

    if (!ready())
        return;

    call_overload_set cs = call_overload_set_dp->get();

    parameter_set_reference psr;
    psr.callee = chain;
    for (auto& argtype : cs.argument_types)
    {
        assert(!qualified_is_contextual(argtype));
        psr.parameters.push_back(argtype);
    }


    set_value(psr);
}
