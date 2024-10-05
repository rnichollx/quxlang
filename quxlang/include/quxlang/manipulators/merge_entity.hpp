// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_MERGE_ENTITY_HEADER_GUARD
#define QUXLANG_MANIPULATORS_MERGE_ENTITY_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    void merge_entity(ast2_symboid& destination, ast2_declarable const& source);
} // namespace quxlang

#endif // QUXLANG_MERGE_ENTITY_HEADER_GUARD
