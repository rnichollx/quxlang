// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTION_DECLARATION_HEADER_GUARD
#define QUXLANG_RES_FUNCTION_DECLARATION_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/macros.hpp"


namespace quxlang
{
   QUX_CO_RESOLVER(function_declaration, selection_reference, std::optional<ast2_function_declaration>);
}

#endif // RPNX_QUXLANG_FUNCTION_DECLARATION_HEADER
