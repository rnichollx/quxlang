// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/function_declaration_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::function_declaration_spec > quxlang::function_declaration_impl(temploid_reference input)
{
    type_symbol functum = input.templexoid;
    std::string name = to_string(functum);

    auto functum_kind = co_await rpnx::querygraph::request< symbol_type_query >(functum);
    if (functum_kind == symbol_kind::template_)
    {
        throw quxlang::compiler_bug("function_declaration received a template selection. Templates of functions are not directly callable; set template arguments manually and resolve the selected function first.");
    }

    auto const& decls = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(functum);
    auto builtin_overloads = co_await rpnx::querygraph::request< functum_builtin_overloads_query >(functum);
    auto const total_count = decls.size() + builtin_overloads.size();

    if (!input.overload_id.has_value())
    {
        if (total_count != 1 || decls.size() != 1)
        {
            co_return std::nullopt;
        }
        co_return decls.front();
    }

    if (*input.overload_id >= decls.size())
    {
        co_return std::nullopt;
    }

    co_return decls.at(static_cast< std::vector< ast2_function_declaration >::size_type >(*input.overload_id));
}
