// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_AST_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_AST_HEADER_GUARD

#include <quxlang/ast2/ast2_module.hpp>
#include <string>


namespace quxlang
{
    struct module_ast_query
    {
        static constexpr auto query_id = "module_ast";
        using input_type = std::string;
        using output_type = ast2_module_declaration;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_AST_HEADER_GUARD
