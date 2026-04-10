// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD

#include <quxlang/queries/instanciation_tempar_map.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using instanciation_tempar_map_spec = rpnx::querygraph::query_handler_spec< instanciation_tempar_map_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< instanciation_tempar_map_spec > instanciation_tempar_map_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD
