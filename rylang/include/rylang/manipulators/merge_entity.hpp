//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RYLANG_MERGE_ENTITY_HEADER_GUARD
#define RYLANG_MERGE_ENTITY_HEADER_GUARD

#include "rylang/ast/entity_ast.hpp"
#include <rylang/ast2/ast2_entity.hpp>

namespace rylang
{
    void merge_entity(entity_ast& destination, entity_ast const& source);
    void merge_entity(ast2_map_entity& destination, ast2_declarable const& source);
} // namespace rylang

#endif // RYLANG_MERGE_ENTITY_HEADER_GUARD
