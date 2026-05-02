// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTION_PACK_INFO_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTION_PACK_INFO_SPEC_HEADER_GUARD

#include <quxlang/queries/function_declaration.hpp>
#include <quxlang/queries/function_pack_info.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    /// Query handler specification for deriving positional pack metadata.
    struct function_pack_info_spec
    {
        using query = function_pack_info_query;
        using dependencies = rpnx::typelist< function_declaration_query >;
    };

    /// Computes source pack-to-expanded-parameter mappings for an instantiated function.
    rpnx::querygraph::coroutine< function_pack_info_spec > function_pack_info_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTION_PACK_INFO_SPEC_HEADER_GUARD
