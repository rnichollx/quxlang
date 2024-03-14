// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef EXTERN_LINKSYMBOL_RESOLVER_HPP
#define EXTERN_LINKSYMBOL_RESOLVER_HPP


#include "quxlang/ast2/ast2_entity.hpp"

#include <quxlang/macros.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(extern_linksymbol, ast2_extern, std::string);
}

#endif  //EXTERN_LINKSYMBOL_RESOLVER_HPP
