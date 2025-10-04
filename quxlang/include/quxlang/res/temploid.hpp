// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TEMPLOID_HEADER_GUARD
#define QUXLANG_RES_TEMPLOID_HEADER_GUARD
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"

namespace quxlang
{
    // Returns a list of user-defined temploid headers for the templexoid
    QUX_CO_RESOLVER(templexoid_user_header_list, type_symbol, std::vector< temploid_header >);

    // Returns the set of user-defined ensigs for the templexoid
    QUX_CO_RESOLVER(templexoid_user_ensig_set, type_symbol, std::set< temploid_ensig >);

    using templexoid_user_ensig_declaroid_map_type = std::map< temploid_ensig, temploid >;
    QUX_CO_RESOLVER(templexoid_user_ensig_declaroid_map, type_symbol, templexoid_user_ensig_declaroid_map_type);

    // Returns the set of builtin ensigs for the templexoid
    QUX_CO_RESOLVER(templexoid_ensig_set, type_symbol, std::set< temploid_ensig >);

    QUX_CO_RESOLVER(temploid_is_builtin, temploid_reference, bool);



} // namespace quxlang

#endif // TEMPLOID_H
