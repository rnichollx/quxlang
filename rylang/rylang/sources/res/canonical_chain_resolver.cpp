//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/res/canonical_chain_resolver.hpp"

// TODO: Implement this resolver
// For now, we don't actually do any processing, but rather assume the chain is already cannonical

void rylang::canonical_chain_resolver::process(compiler* c)
{
    canonical_lookup_chain output;
    for (auto const& i : m_chain.chain)
    {
        // TODO: Later handle non-scope stuff
        output.push_back(i.identifier);
    }
    set_value(output);
}