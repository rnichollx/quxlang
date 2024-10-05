// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#ifndef QUXLANG_RES_EXTERN_LINKSYMBOL_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_EXTERN_LINKSYMBOL_RESOLVER_HEADER_GUARD


#include "quxlang/ast2/ast2_entity.hpp"

#include <quxlang/macros.hpp>
namespace quxlang
{
    QUX_CO_RESOLVER(extern_linksymbol, ast2_extern, std::string);
}

#endif  //EXTERN_LINKSYMBOL_RESOLVER_HPP
