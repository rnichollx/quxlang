//
// Created by Ryan Nicholl on 9/20/23.
//

#ifndef RYLANG_MERGE_ENTITY_HEADER_GUARD
#define RYLANG_MERGE_ENTITY_HEADER_GUARD

#include "rylang/ast/entity_ast.hpp"

namespace rylang
{
   void merge_entity(entity_ast & destination, entity_ast const & source);
}

#endif // RYLANG_MERGE_ENTITY_HEADER_GUARD
