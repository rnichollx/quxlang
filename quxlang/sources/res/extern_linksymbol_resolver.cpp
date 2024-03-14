// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL


#include <quxlang/res/extern_linksymbol_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(extern_linksymbol)
{
    auto lang = input_val.lang;

    if (lang == "C")
    {
        // TODO: Resolve this to multiple formats of symbols based on the platform, not all OS prefix C symbols with an underscore.

        QUX_CO_ANSWER("_" + input_val.symbol);
    }
    else if (lang == "LINKER")
    {
        QUX_CO_ANSWER(input_val.symbol);
    }
    else
    {
        throw std::runtime_error("Extern language '" + lang + "' not supported");
    }
}