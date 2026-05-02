// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_DECLARATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_DECLARATION_SPEC_HEADER_GUARD

#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/functum_map_user_formal_ensigs.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct function_declaration_spec
    {
        using query = function_declaration_query;
        using dependencies = rpnx::typelist< functum_list_user_overload_declarations_query, functum_map_user_formal_ensigs_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< function_declaration_spec > function_declaration_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_DECLARATION_SPEC_HEADER_GUARD
