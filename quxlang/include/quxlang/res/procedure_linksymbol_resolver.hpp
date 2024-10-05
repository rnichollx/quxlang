// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_RES_PROCEDURE_LINKSYMBOL_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_PROCEDURE_LINKSYMBOL_RESOLVER_HEADER_GUARD


#include "quxlang/data/type_symbol.hpp"

#include <quxlang/macros.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(procedure_linksymbol, ast2_procedure_ref, std::string);
}

#endif //PROCEDURE_LINKSYMBOL_RESOLVER_HPP
