// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_BUILTIN_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_BUILTIN_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD

#include <new>

#include <rpnx/querygraph/querygraph.hpp>

#include <quxlang/queries/builtin_template_instanciation.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/ensig_initialize.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

namespace quxlang
{
    struct builtin_template_instanciation_spec
    {
        using query = builtin_template_instanciation_query;
        using dependencies = rpnx::typelist< constexpr_eval_v3_query, ensig_initialize_query, lookup_query, symbol_type_query, template_builtin_query, temploid_formal_ensig_query >;
    };

    rpnx::querygraph::coroutine< builtin_template_instanciation_spec > builtin_template_instanciation_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_BUILTIN_TEMPLATE_INSTANCIATION_SPEC_HEADER_GUARD
