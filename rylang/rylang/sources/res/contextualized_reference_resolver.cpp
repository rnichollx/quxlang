//
// Created by Ryan Nicholl on 11/5/23.
//

#include "rylang/res/contextualized_reference_resolver.hpp"
#include "rylang/manipulators/qualified_reference.hpp"


void rylang::contextualized_reference_resolver::process(compiler* c)
{
   set_value(with_context(m_symbol, m_context));
}
