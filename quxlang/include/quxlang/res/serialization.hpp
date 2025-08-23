// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL

#ifndef QUXLANG_RES_SERIALIZATION_HPP
#define QUXLANG_RES_SERIALIZATION_HPP


#include <quxlang/res/resolver.hpp>
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{

    QUX_CO_RESOLVER(user_serialize_exists, type_symbol, bool);
    QUX_CO_RESOLVER(user_deserialize_exists, type_symbol, bool);
    QUX_CO_RESOLVER(type_is_implicitly_datatype, type_symbol, bool);
    QUX_CO_RESOLVER(type_should_autogen_serialize, type_symbol, bool);
    QUX_CO_RESOLVER(type_should_autogen_deserialize, type_symbol, bool);


}


#endif //SERIALIZATION_H
