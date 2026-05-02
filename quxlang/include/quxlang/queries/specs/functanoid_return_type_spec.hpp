// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTANOID_RETURN_TYPE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTANOID_RETURN_TYPE_SPEC_HEADER_GUARD

#include <quxlang/queries/functanoid_return_type.hpp>
#include <quxlang/queries/functanoid_deduced_return_type.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_primitive.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/user_vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functanoid_return_type_spec
    {
        using query = functanoid_return_type_query;
        using dependencies = rpnx::typelist< function_declaration_query, function_primitive_query, functanoid_deduced_return_type, lookup_query, user_vm_procedure3_query >;
    };

    rpnx::querygraph::coroutine< functanoid_return_type_spec > functanoid_return_type_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTANOID_RETURN_TYPE_SPEC_HEADER_GUARD
