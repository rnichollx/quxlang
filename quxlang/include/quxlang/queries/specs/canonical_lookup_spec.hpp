// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CANONICAL_LOOKUP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CANONICAL_LOOKUP_SPEC_HEADER_GUARD

#include <quxlang/queries/canonical_lookup.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/exists.hpp>
#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_pack_info.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/subtag_binding.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symbol_type.hpp>

#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /** Querygraph handler specification for access-neutral lookup. */
    struct canonical_lookup_spec
    {
        using query = canonical_lookup_query;
        using dependencies = rpnx::typelist< canonical_lookup_query, constexpr_u64_query, exists_query, function_declaration_query, function_pack_info_query, instanciation_query, machine_info_query, module_ast_query, subtag_binding_query, symboid_query, symbol_type_query >;
    };

    /** Implements access-neutral canonical type lookup. */
    rpnx::querygraph::coroutine< canonical_lookup_spec > canonical_lookup_impl(contextual_type_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CANONICAL_LOOKUP_SPEC_HEADER_GUARD
