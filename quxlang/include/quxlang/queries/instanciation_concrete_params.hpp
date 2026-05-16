// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INSTANCIATION_CONCRETE_PARAMS_HEADER_GUARD
#define QUXLANG_QUERIES_INSTANCIATION_CONCRETE_PARAMS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Resolves the concrete runtime parameter surface for a canonical instantiated overload.
    struct instanciation_concrete_params_query
    {
        static constexpr auto query_id = "instanciation_concrete_params";
        using input_type = instanciation_reference;
        using output_type = instatype;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INSTANCIATION_CONCRETE_PARAMS_HEADER_GUARD
