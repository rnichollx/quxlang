//
// Created by Ryan Nicholl on 11/4/23.
//
#include "rylang/res/function_qualified_reference_resolver.hpp"
#include "rylang/converters/qual_converters.hpp"

void rylang::function_qualified_reference_resolver::process(compiler* c)
{
    // TODO: Lookup chain is deprecated, but this is being used by other resolvers still
    canonical_lookup_chain chain = convert_to_canonical_lookup_chain(m_input);

    auto call_overload_set_dp = get_dependency(
        [&]
        {
            return c->lk_function_overload_selection(chain, m_args);
        });

    if (!ready())
        return;

    call_overload_set cs = call_overload_set_dp->get();


    qualified_symbol_reference output = convert_to_qualified_symbol_reference(chain, cs);

    output = parameter_set_reference{output, cs};
    // TODO:
}
