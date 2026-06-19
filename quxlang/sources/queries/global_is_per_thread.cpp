// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/queries/specs/global_is_per_thread_spec.hpp>

rpnx::querygraph::coroutine< quxlang::global_is_per_thread_spec > quxlang::global_is_per_thread_impl(type_symbol input)
{
    symbol_kind kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    if (kind != symbol_kind::global_variable)
    {
        co_return false;
    }

    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        co_return false;
    }

    ast2_variable_declaration const& decl = as< ast2_variable_declaration >(symboid);
    co_return decl.keyword_tags.contains("PER_THREAD");
}
