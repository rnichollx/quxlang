// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_PROCEDURE_LINKSYMBOL_HEADER_GUARD
#define QUXLANG_QUERIES_PROCEDURE_LINKSYMBOL_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <string>


namespace quxlang
{
    struct procedure_linksymbol_query
    {
        static constexpr auto query_id = "procedure_linksymbol";
        using input_type = ast2_procedure_ref;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_PROCEDURE_LINKSYMBOL_HEADER_GUARD
