// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INSTANCIATION_TEMPAR_MAP_HEADER_GUARD
#define QUXLANG_QUERIES_INSTANCIATION_TEMPAR_MAP_HEADER_GUARD

#include <quxlang/data/temploid_instanciation_parameter_set.hpp>
#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    struct instanciation_tempar_map_query
    {
        static constexpr auto query_id = "instanciation_tempar_map";
        using input_type = instanciation_reference;
        using output_type = temploid_instanciation_parameter_set;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INSTANCIATION_TEMPAR_MAP_HEADER_GUARD
