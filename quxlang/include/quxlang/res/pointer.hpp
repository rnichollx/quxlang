//
// Created by rnicholl on 4/28/25.
//

#ifndef POINTER_HPP
#define POINTER_HPP
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"
namespace quxlang
{
    QUX_CO_RESOLVER(uintpointer_type, std::monostate, type_symbol );
    QUX_CO_RESOLVER(sintpointer_type, std::monostate, type_symbol );
}

#endif //POINTER_HPP
