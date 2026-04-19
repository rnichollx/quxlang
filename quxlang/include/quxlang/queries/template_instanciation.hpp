// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLATE_INSTANCIATION_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLATE_INSTANCIATION_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <optional>


namespace quxlang
{
    struct template_instanciation_query
    {
        static constexpr auto query_id = "template_instanciation";
        using input_type = initialization_reference;
        using output_type = std::optional< instanciation_reference >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLATE_INSTANCIATION_HEADER_GUARD
