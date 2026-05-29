// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/global_is_antestatal_static_spec.hpp>

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
    if (typeis< readonly_constant >(variable_type))
    {
        readonly_constant const& constant_type = as< readonly_constant >(variable_type);
        if (constant_type.kind == constant_kind::string)
        {
            co_return true;
        }
    }
    if (!(co_await rpnx::querygraph::request< type_is_antestatal_query >(variable_type)))
    {
        auto type_kind = co_await rpnx::querygraph::request< symbol_type_query >(variable_type);
        if (type_kind == symbol_kind::class_)
        {
            auto tags = co_await rpnx::querygraph::request< class_tags_query >(variable_type);
            if (tags.contains(keywords::nonstatic))
            {
                throw semantic_compilation_error("STATIC global has a NONSTATIC type: " + quxlang::to_string(input));
            }
        }
        co_return false;
    }

    co_return true;
}
