// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/procedure_linksymbol_spec.hpp>

#include "quxlang/manipulators/typeutils.hpp"

rpnx::querygraph::coroutine< quxlang::procedure_linksymbol_spec > quxlang::procedure_linksymbol_impl(ast2_procedure_ref input)
{
    co_return to_string(input.functanoid);
}
