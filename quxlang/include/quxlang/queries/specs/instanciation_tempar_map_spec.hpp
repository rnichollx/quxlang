// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD

#include <quxlang/queries/instanciation_tempar_map.hpp>
#include <quxlang/queries/temploid_formal_ensig.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct instanciation_tempar_map_spec
    {
        using query = instanciation_tempar_map_query;
        using dependencies = rpnx::typelist< temploid_formal_ensig_query >;
    };

    rpnx::querygraph::coroutine< instanciation_tempar_map_spec > instanciation_tempar_map_impl(instanciation_reference input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INSTANCIATION_TEMPAR_MAP_SPEC_HEADER_GUARD
