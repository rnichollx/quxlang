// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_PACK_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_PACK_INFO_HEADER_GUARD

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <rpnx/macros.hpp>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Concrete metadata for one positional pack in a fully selected function instantiation.
    struct function_pack_entry
    {
        /// Count of ordinary positional parameters before the pack starts.
        std::uint64_t fixed_prefix_count = 0;
        /// Number of concrete positional arguments captured by the pack.
        std::uint64_t size = 0;
        /// Concrete argument types captured by the pack in source order.
        std::vector< type_symbol > types;
        /// Expanded VMIR positional parameter indices for each captured pack argument.
        std::vector< std::uint64_t > positional_indices;

        RPNX_MEMBER_METADATA(function_pack_entry, fixed_prefix_count, size, types, positional_indices);
    };

    /// Pack metadata for a fully selected function instantiation, keyed by source pack name.
    struct function_pack_info
    {
        /// Positional pack entries visible in the function body.
        std::map< std::string, function_pack_entry > packs;

        RPNX_MEMBER_METADATA(function_pack_info, packs);
    };

    /// Derives positional pack metadata from an instantiated function reference.
    struct function_pack_info_query
    {
        /// Stable query identifier for pack metadata derivation.
        static constexpr auto query_id = "function_pack_info";
        /// Fully instantiated function whose selected declaration is inspected.
        using input_type = instanciation_reference;
        /// Concrete pack metadata for the instantiated function.
        using output_type = function_pack_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_PACK_INFO_HEADER_GUARD
