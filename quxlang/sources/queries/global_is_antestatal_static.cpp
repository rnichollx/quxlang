// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/keywords.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/global_is_antestatal_static_spec.hpp>

rpnx::querygraph::coroutine< quxlang::global_is_antestatal_static_spec > quxlang::global_is_antestatal_static_impl(type_symbol input)
{
    if (typeis< subtag_type >(input))
    {
        auto binding = co_await rpnx::querygraph::request< subtag_binding_query >(as< subtag_type >(input));
        co_return binding.has_value() && binding->template type_is< parameter_value_instantiation >();
    }

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
        if (constant_type.kind == constant_kind::numeric)
        {
            co_return true;
        }
    }
    if (!(co_await rpnx::querygraph::request< type_is_antestatal_query >(variable_type)))
    {
        class_kind const type_kind = co_await rpnx::querygraph::request< class_type_query >(variable_type);
        if (type_kind == class_kind::struct_)
        {
            struct_tags_result_type const tags = co_await rpnx::querygraph::request< struct_tags_query >(variable_type);
            if (tags.contains(keywords::nonstatic))
            {
                throw semantic_compilation_error("STATIC global has a NONSTATIC type: " + quxlang::to_string(input));
            }
        }
        co_return false;
    }

    co_return true;
}
