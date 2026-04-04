// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>

#include "quxlang/manipulators/mangler.hpp"




rpnx::querygraph::coroutine< quxlang::procedure_linksymbol_spec > quxlang::procedure_linksymbol_impl(ast2_procedure_ref input)
{
    // TODO: Support calling convention?
    std::string input_text = to_string(input.functanoid);

    std::string result = mangle(input.functanoid);
    co_return result;
}
