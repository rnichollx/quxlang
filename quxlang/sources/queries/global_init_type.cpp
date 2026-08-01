// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/queries/specs/global_init_type_spec.hpp>

rpnx::querygraph::coroutine< quxlang::global_init_type_spec > quxlang::global_init_type_impl(type_symbol input)
{
    if (input.type_is< builtin_symbol >())
    {
        std::string const& name = input.get_as< builtin_symbol >().name;
        if (name == "STEPPING_COUNT" || name == "ACTIVE_STEPPING" ||
            name == "MAIN_FUNCTION_ARRAY" || name == "POST_DETECT_FUNCTION_ARRAY" ||
            name == "UNIT_TEST_COUNT" || name == "UNIT_TEST_NAMES" || name == "UNIT_TEST_PROC" ||
            is_cpu_attribute_enabled_name(name))
        {
            co_return initialization_type::init_compiler_builtin;
        }
    }

    if ((co_await rpnx::querygraph::request< symbol_type_query >(input)) != symbol_kind::global_variable)
    {
        co_return initialization_type::init_with_guard;
    }

    ast2_symboid symboid = co_await rpnx::querygraph::request< symboid_query >(input);
    if (!typeis< ast2_variable_declaration >(symboid))
    {
        co_return initialization_type::init_with_guard;
    }

    ast2_variable_declaration const& decl = as< ast2_variable_declaration >(symboid);
    if (decl.init_expr.has_value() || !decl.init_args.empty())
    {
        co_return initialization_type::init_with_guard;
    }

    type_symbol const variable_type = co_await rpnx::querygraph::request< variable_type_query >(input);
    if (co_await rpnx::querygraph::request< type_is_trivially_default_constructible_query >(variable_type))
    {
        co_return initialization_type::init_trivial;
    }

    co_return initialization_type::init_with_guard;
}
