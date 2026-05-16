// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/template_builtin_spec.hpp>

using namespace quxlang;

rpnx::querygraph::coroutine< quxlang::template_builtin_spec > quxlang::template_builtin_impl(temploid_reference input)
{
    auto builtin_templates = co_await rpnx::querygraph::request< templex_builtins_query >(input.templexoid);

    if (!input.overload_id.has_value())
    {
        co_return builtin_templates.size() == 1;
    }

    co_return *input.overload_id < builtin_templates.size();
}
