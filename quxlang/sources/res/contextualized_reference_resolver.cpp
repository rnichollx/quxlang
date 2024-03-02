//
// Created by Ryan Nicholl on 11/5/23.
//
#include "quxlang/compiler.hpp"
#include "quxlang/res/contextualized_reference_resolver.hpp"
#include "quxlang/manipulators/qmanip.hpp"

void quxlang::contextualized_reference_resolver::process(compiler* c)
{
   set_value(with_context(m_symbol, m_context));
}
