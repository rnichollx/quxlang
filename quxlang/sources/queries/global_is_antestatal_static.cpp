// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/global_is_antestatal_static_spec.hpp>

#include <stdexcept>


rpnx::querygraph::coroutine< quxlang::global_is_antestatal_static_spec > quxlang::global_is_antestatal_static_impl(type_symbol input)
{
    auto kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (kind != symbol_kind::global_variable)
    {
        co_return false;
    }

    auto symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        co_return false;
    }

    auto const& decl = as< ast2_variable_declaration >(symboid);
    if (!decl.keyword_tags.contains("STATIC"))
    {
        co_return false;
    }

    auto variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);
    if (!(co_await rpnx::querygraph::request< type_is_antestatal_query >(variable_type)))
    {
        throw std::logic_error("STATIC global has a non-antestatal type: " + quxlang::to_string(input));
    }

    co_return true;
}
