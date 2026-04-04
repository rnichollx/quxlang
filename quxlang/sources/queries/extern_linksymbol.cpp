// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/extern_linksymbol_spec.hpp>



rpnx::querygraph::coroutine< quxlang::extern_linksymbol_spec > quxlang::extern_linksymbol_impl(ast2_extern input)
{
    auto lang = input.lang;

    if (lang == "C")
    {
        // TODO: Resolve this to multiple formats of symbols based on the platform, not all OS prefix C symbols with an underscore.

        co_return "_" + input.symbol;
    }
    else if (lang == "LINKER")
    {
        co_return input.symbol;
    }
    else
    {
        throw std::logic_error("Extern language '" + lang + "' not supported");
    }
}