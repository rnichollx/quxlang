// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_CLASS_FIELD_LIST_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_CLASS_FIELD_LIST_RESOLVER_HEADER_GUARD

// input: canonical_lookup_chain
// output: class_field_list

#include "quxlang/compiler_fwd.hpp"
#include "rpnx/resolver_utilities.hpp"

#include "quxlang/data/class_field_declaration.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(class_field_declaration_list, type_symbol, std::vector< class_field_declaration >);
    QUX_CO_RESOLVER(class_field_list, type_symbol, std::vector< class_field >);
} // namespace quxlang

#endif // QUXLANG_CLASS_FIELD_LIST_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
