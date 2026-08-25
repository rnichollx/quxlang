// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLATE_PARAMETER_BINDING_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLATE_PARAMETER_BINDING_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <optional>
#include <string>

namespace quxlang
{
    /** Identifies a template parameter name relative to an instantiated context. */
    struct template_parameter_binding_input
    {
        /// Innermost instantiated context in which the name appears.
        type_symbol context;
        /// Source-level template parameter name.
        std::string name;

        RPNX_MEMBER_METADATA(template_parameter_binding_input, context, name);
    };

    /** Finds the nearest instantiated template parameter binding with a given name. */
    struct template_parameter_binding_query
    {
        static constexpr auto query_id = "template_parameter_binding";
        using input_type = template_parameter_binding_input;
        using output_type = std::optional< parameter_instantiation >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLATE_PARAMETER_BINDING_HEADER_GUARD
