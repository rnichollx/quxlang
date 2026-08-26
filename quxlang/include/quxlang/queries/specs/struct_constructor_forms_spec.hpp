// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_STRUCT_CONSTRUCTOR_FORMS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_STRUCT_CONSTRUCTOR_FORMS_SPEC_HEADER_GUARD

#include <quxlang/queries/active_symboid_subdeclaroids.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/struct_constructor_forms.hpp>
#include <quxlang/queries/struct_inheritance_info.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct struct_constructor_forms_spec
    {
        using query = struct_constructor_forms_query;
        using dependencies = rpnx::typelist< active_symboid_subdeclaroids_query, lookup_query, struct_inheritance_info_query >;
    };

    /** Normalizes constructor templates and explicit full/subobject pairs. */
    auto struct_constructor_forms_impl(type_symbol input) -> rpnx::querygraph::coroutine< struct_constructor_forms_spec >;
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_STRUCT_CONSTRUCTOR_FORMS_SPEC_HEADER_GUARD
