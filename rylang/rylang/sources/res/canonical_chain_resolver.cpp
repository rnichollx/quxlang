//
// Created by Ryan Nicholl on 9/20/23.
//

#include "rylang/res/canonical_chain_resolver.hpp"

// TODO: Implement this resolver
// For now, we don't actually do any processing, but rather assume the chain is already cannonical

void rylang::canonical_chain_resolver::process(compiler* c)
{
  set_value(m_chain);
}