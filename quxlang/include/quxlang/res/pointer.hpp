// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_POINTER_HEADER_GUARD
#define QUXLANG_RES_POINTER_HEADER_GUARD

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
