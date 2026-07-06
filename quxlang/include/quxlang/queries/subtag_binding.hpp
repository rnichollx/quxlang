// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SUBTAG_BINDING_HEADER_GUARD
#define QUXLANG_QUERIES_SUBTAG_BINDING_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <optional>

namespace quxlang
{
    /// Resolves one subtag symbol to the template parameter binding that it exposes.
    struct subtag_binding_query
    {
        /// Stable querygraph identifier.
        static constexpr auto query_id = "subtag_binding";
        /// Subtag symbol to resolve.
        using input_type = subtag_type;
        /// Exposed template parameter binding, or nullopt when the subtag is not valid.
        using output_type = std::optional< parameter_instantiation >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SUBTAG_BINDING_HEADER_GUARD
