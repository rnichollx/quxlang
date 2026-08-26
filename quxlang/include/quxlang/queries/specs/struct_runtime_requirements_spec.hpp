// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_REQUIREMENTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_REQUIREMENTS_SPEC_HEADER_GUARD

#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/struct_constructor_forms.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>
#include <quxlang/queries/struct_runtime_requirements.hpp>
#include <quxlang/queries/struct_tags.hpp>
#include <quxlang/queries/struct_virtual_slots.hpp>
#include <quxlang/queries/symboid.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_runtime_requirements_spec
    {
        using query = struct_runtime_requirements_query;
        using dependencies = rpnx::typelist< class_type_query, struct_constructor_forms_query, struct_inheritance_info_query, struct_tags_query, struct_virtual_slots_query, symboid_query >;
    };

    /** Returns layout-independent runtime-header, RTTI, and destructor requirements. */
    auto struct_runtime_requirements_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_runtime_requirements_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_RUNTIME_REQUIREMENTS_SPEC_HEADER_GUARD
