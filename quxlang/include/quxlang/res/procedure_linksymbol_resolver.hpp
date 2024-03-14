// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef PROCEDURE_LINKSYMBOL_RESOLVER_HPP
#define PROCEDURE_LINKSYMBOL_RESOLVER_HPP


#include "quxlang/data/type_symbol.hpp"

#include <quxlang/macros.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(procedure_linksymbol, ast2_procedure_ref, std::string);
}

#endif //PROCEDURE_LINKSYMBOL_RESOLVER_HPP
