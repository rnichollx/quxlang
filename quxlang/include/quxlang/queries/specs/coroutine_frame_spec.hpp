// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_COROUTINE_FRAME_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_COROUTINE_FRAME_SPEC_HEADER_GUARD

#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/coroutine_frame.hpp>
#include <quxlang/queries/function_builtin.hpp>
#include <quxlang/queries/instanciation_concrete_params.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/struct_direct_bases.hpp>
#include <quxlang/queries/struct_field_declaration_list.hpp>
#include <quxlang/queries/type_is_trivially_relocatable.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/vm_procedure3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Declares semantic-body and target-placement dependencies of coroutine frame construction. */
    struct coroutine_frame_spec
    {
        using query = coroutine_frame_query;
        using dependencies = rpnx::typelist< vm_procedure3_query, function_builtin_query, class_placement_info_query, machine_info_query, type_is_trivially_relocatable_query, class_type_query, lookup_query, instanciation_concrete_params_query, struct_direct_bases_query,
            struct_field_declaration_list_query, union_info_query, variant_info_query >;
    };

    /** Constructs typed persistent storage from the logical coroutine CFG. */
    rpnx::querygraph::coroutine< coroutine_frame_spec > coroutine_frame_impl(instanciation_reference input);
}

#endif // QUXLANG_QUERIES_SPECS_COROUTINE_FRAME_SPEC_HEADER_GUARD
