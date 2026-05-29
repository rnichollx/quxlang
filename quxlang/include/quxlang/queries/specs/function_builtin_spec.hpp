// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD

#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/functum_builtin_overloads.hpp>
#include <quxlang/queries/functum_map_user_formal_ensigs.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct function_builtin_spec
    {
        using query = function_builtin_query;
        using dependencies = rpnx::typelist< functum_builtin_overloads_query, functum_map_user_formal_ensigs_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< function_builtin_spec > function_builtin_impl(temploid_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_BUILTIN_SPEC_HEADER_GUARD
