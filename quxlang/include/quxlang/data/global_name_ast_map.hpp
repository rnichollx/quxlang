//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef QUXLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD
#define QUXLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD

#include "quxlang/ast/entity_ast.hpp"

namespace quxlang
{
   struct global_name_ast_map
   {
      std::map<std::string, entity_ast> map;
   };
}

#endif // QUXLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD
