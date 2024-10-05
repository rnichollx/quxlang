// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_GLOBAL_NAME_AST_MAP_HEADER_GUARD
#define QUXLANG_DATA_GLOBAL_NAME_AST_MAP_HEADER_GUARD

#include "quxlang/ast/entity_ast.hpp"

namespace quxlang
{
   struct global_name_ast_map
   {
      std::map<std::string, entity_ast> map;
   };
}

#endif // QUXLANG_GLOBAL_NAME_AST_MAP_HEADER_GUARD
