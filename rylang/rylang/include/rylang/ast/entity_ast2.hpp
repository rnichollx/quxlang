//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_ENTITY_AST2_HEADER
#define RPNX_RYANSCRIPT1031_ENTITY_AST2_HEADER

#include <rpnx/value.hpp>

namespace rylang
{
   using entity_ast2 = rpnx::value<std::variant<null_object_ast, function_entity_ast, class_entity_ast> >;
}

#endif // RPNX_RYANSCRIPT1031_ENTITY_AST2_HEADER
