//
// Created by Ryan Nicholl on 8/15/23.
//

#ifndef RPNX_RYANSCRIPT1031_GLOBAL_NAME_AST_MAP_HEADER
#define RPNX_RYANSCRIPT1031_GLOBAL_NAME_AST_MAP_HEADER

#include "rylang/ast/entity_ast.hpp"

namespace rylang
{
   struct global_name_ast_map
   {
      std::map<std::string, entity_ast> map;
   };
}

#endif // RPNX_RYANSCRIPT1031_GLOBAL_NAME_AST_MAP_HEADER
