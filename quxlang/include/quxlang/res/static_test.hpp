//
// Created by rnicholl on 5/21/25.
//

#ifndef QUXLANG_RES_STATIC_TEST_HEADER_GUARD
#define QUXLANG_RES_STATIC_TEST_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(list_static_tests, type_symbol, std::set< type_symbol >);

}

#endif //QUXLANG_RES_STATIC_TEST_HPP
