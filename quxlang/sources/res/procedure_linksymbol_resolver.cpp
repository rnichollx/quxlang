// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL


#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/res/procedure_linksymbol_resolver.hpp>


QUX_CO_RESOLVER_IMPL_FUNC_DEF(procedure_linksymbol)
{
    // TODO: Support calling convention?
    auto input = input_val;

    std::string input_text = to_string(input_val.functanoid);

    std::string result = mangle(input_val.functanoid);
    QUX_CO_ANSWER(result);
}