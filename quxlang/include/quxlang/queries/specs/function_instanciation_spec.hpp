// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD

#include <quxlang/queries/function_instanciation.hpp>
#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct function_instanciation_spec
    {
        using query = function_instanciation_query;
        using dependencies = rpnx::typelist< function_ensig_init_with_query, symbol_type_query, temploid_formal_ensig_query >;
    };

    rpnx::querygraph::coroutine< function_instanciation_spec > function_instanciation_impl(initialization_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_INSTANCIATION_SPEC_HEADER_GUARD
