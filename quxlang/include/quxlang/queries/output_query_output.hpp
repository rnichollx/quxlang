// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_QUERY_OUTPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_QUERY_OUTPUT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/target_configuration.hpp>

#include <rpnx/macros.hpp>

#include <optional>
#include <string>
#include <vector>

namespace quxlang
{
    /**
     * Describes one configured qxc output entry.
     */
    struct output_query_output
    {
        std::string output_name;
        std::vector< std::string > module_names;
        std::optional< type_symbol > main_functanoid;
        output_kind type = output_kind::executable;

        RPNX_MEMBER_METADATA(output_query_output, output_name, module_names, main_functanoid, type);
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_QUERY_OUTPUT_HEADER_GUARD
