//
// Created by Ryan Nicholl on 4/6/24.
//

#ifndef RPNX_QUXLANG_FUNCTION_DECLARATION_HEADER
#define RPNX_QUXLANG_FUNCTION_DECLARATION_HEADER

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/macros.hpp"


namespace quxlang
{
   QUX_CO_RESOLVER(function_declaration, selection_reference, std::optional<ast2_function_declaration>);
}

#endif // RPNX_QUXLANG_FUNCTION_DECLARATION_HEADER
