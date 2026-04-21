// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/template_builtin_spec.hpp>

using namespace quxlang;

rpnx::querygraph::coroutine< quxlang::template_builtin_spec > quxlang::template_builtin_impl(temploid_reference input)
{
    auto builtin_templates = co_await rpnx::querygraph::request< templex_builtin_templates_query >(input.templexoid);

    for (auto const& info : builtin_templates)
    {
        if (info == input.which)
        {
            co_return true;
        }
    }

    co_return false;
}
