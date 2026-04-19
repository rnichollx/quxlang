// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_EXTERN_LINKSYMBOL_HEADER_GUARD
#define QUXLANG_QUERIES_EXTERN_LINKSYMBOL_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <string>


namespace quxlang
{
    struct extern_linksymbol_query
    {
        static constexpr auto query_id = "extern_linksymbol";
        using input_type = ast2_extern;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_EXTERN_LINKSYMBOL_HEADER_GUARD
