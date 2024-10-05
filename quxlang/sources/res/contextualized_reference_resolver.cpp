// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"
#include "quxlang/res/contextualized_reference_resolver.hpp"
#include "quxlang/manipulators/qmanip.hpp"

void quxlang::contextualized_reference_resolver::process(compiler* c)
{
   set_value(with_context(m_symbol, m_context));
}
