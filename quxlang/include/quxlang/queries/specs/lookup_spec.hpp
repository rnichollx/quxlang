// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD

#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/exists.hpp>
#include <quxlang/queries/function_pack_info.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct lookup_spec
    {
        using query = lookup_query;
        using dependencies = rpnx::typelist< constexpr_u64_query, exists_query, function_declaration_query, function_pack_info_query, instanciation_query, lookup_query, machine_info_query, module_ast_query, subtag_binding_query, symboid_query, symbol_type_query >;
    };

    rpnx::querygraph::coroutine< lookup_spec > lookup_impl(contextual_type_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LOOKUP_SPEC_HEADER_GUARD
