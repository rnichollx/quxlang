// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTINS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTINS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_builtins.hpp>
#include <quxlang/queries/class_requires_gen_assignment.hpp>
#include <quxlang/queries/class_requires_gen_swap.hpp>
#include <quxlang/queries/functum_user_overloads.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/global_is_serialoid_static.hpp>
#include <quxlang/queries/global_is_string_static.hpp>
#include <quxlang/queries/list_builtin_constructors.hpp>
#include <quxlang/queries/sintpointer_type.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/type_is_implicitly_datatype.hpp>
#include <quxlang/queries/type_should_autogen_deserialize.hpp>
#include <quxlang/queries/type_should_autogen_serialize.hpp>
#include <quxlang/queries/uintpointer_type.hpp>
#include <quxlang/queries/variable_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_builtins_spec
    {
        using query = functum_builtins_query;
        using dependencies = rpnx::typelist< class_requires_gen_assignment_query, class_requires_gen_swap_query, functum_user_overloads_query, global_is_antestatal_static_query, global_is_serialoid_static_query, global_is_string_static_query, list_builtin_constructors_query, sintpointer_type_query, symbol_type_query, template_builtin_query, type_is_implicitly_datatype_query, type_should_autogen_deserialize_query, type_should_autogen_serialize_query, uintpointer_type_query, variable_type_query >;
    };

    rpnx::querygraph::coroutine< functum_builtins_spec > functum_builtins_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_BUILTINS_SPEC_HEADER_GUARD
