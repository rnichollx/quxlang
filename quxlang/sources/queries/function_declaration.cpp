// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/function_declaration_spec.hpp>

#include "quxlang/operators.hpp"


rpnx::querygraph::coroutine< quxlang::function_declaration_spec > quxlang::function_declaration_impl(temploid_reference input)
{
    type_symbol functum = input.templexoid;
    std::string name = to_string(functum);

    auto functum_kind = co_await rpnx::querygraph::request< symbol_type_query >(functum);
    if (functum_kind == symbol_kind::template_)
    {
        throw std::logic_error("function_declaration received a template selection. Templates of functions are not directly callable; set template arguments manually and resolve the selected function first.");
    }

    auto const& decl_map = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(functum);


    if (!decl_map.contains(input.which))
    {
        co_return std::nullopt;
    }

    std::size_t index = decl_map.at(input.which);

    auto const& decls = co_await rpnx::querygraph::request< functum_list_user_overload_declarations_query >(functum);

    co_return decls.at(index);
}
