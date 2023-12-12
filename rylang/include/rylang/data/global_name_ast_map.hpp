//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef RYLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD
#define RYLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD

#include "rylang/ast/entity_ast.hpp"

namespace rylang
{
   struct global_name_ast_map
   {
      std::map<std::string, entity_ast> map;
   };
}

#endif // RYLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD
