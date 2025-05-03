//
// Created by Ryan Nicholl on 2025-04-30.
//

#ifndef POINTER_HPP
#define POINTER_HPP

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    // Both of these take no input and return the int type which matches the pointer size
    QUX_CO_RESOLVER(uintpointer_type, std::monostate, type_symbol);
    QUX_CO_RESOLVER(sintpointer_type, std::monostate, type_symbol);
}

#endif //POINTER_HPP
