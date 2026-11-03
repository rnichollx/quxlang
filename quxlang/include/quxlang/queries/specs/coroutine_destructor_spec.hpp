// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_COROUTINE_DESTRUCTOR_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_COROUTINE_DESTRUCTOR_HEADER_GUARD

#include <quxlang/queries/class_default_dtor.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/coroutine_destructor.hpp>
#include <quxlang/queries/functum_initialize.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_field_list.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/user_default_dtor_exists.hpp>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares dependencies for controller-specialized lifecycle selection. */
    struct coroutine_destructor_spec
    {
        using query = coroutine_destructor_query;
        using dependencies = rpnx::typelist< class_default_dtor_query, class_type_query, function_declaration_query, functum_initialize_query, functum_list_user_overload_declarations_query, struct_direct_bases_query, struct_field_list_query, union_info_query, user_default_dtor_exists_query, variant_info_query >;
    };

    /** Selects the destructor body without allocating or initializing its coroutine frame. */
    rpnx::querygraph::coroutine< coroutine_destructor_spec > coroutine_destructor_impl(coroutine_destructor_request input);
}

#endif
