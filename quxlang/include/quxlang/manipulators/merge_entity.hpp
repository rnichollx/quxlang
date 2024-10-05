//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef QUXLANG_MANIPULATORS_MERGE_ENTITY_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MERGE_ENTITY_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    void merge_entity(ast2_symboid& destination, ast2_declarable const& source);
} // namespace quxlang

#endif // QUXLANG_MERGE_ENTITY_HEADER_GUARD
