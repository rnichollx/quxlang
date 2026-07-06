// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INSTANCIATION_SUBTAG_BINDINGS_HEADER_GUARD
#define QUXLANG_QUERIES_INSTANCIATION_SUBTAG_BINDINGS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <map>
#include <string>

namespace quxlang
{
    /// Lists template parameter bindings that are exposed as subtags on one instantiation.
    struct instanciation_subtag_bindings_query
    {
        /// Stable querygraph identifier.
        static constexpr auto query_id = "instanciation_subtag_bindings";
        /// Instantiation whose exposed template parameter tags are requested.
        using input_type = instanciation_reference;
        /// Map from exposed tag name to the instantiated type or value parameter.
        using output_type = std::map< std::string, parameter_instantiation >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INSTANCIATION_SUBTAG_BINDINGS_HEADER_GUARD
