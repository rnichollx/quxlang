// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_MODULE_AST_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_MODULE_AST_SPEC_HEADER_GUARD

#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/parse_file.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using module_ast_spec = rpnx::querygraph::query_handler_spec< module_ast_query, rpnx::typelist< module_source_name_query, module_sources_query, parse_file_query > >;

    rpnx::querygraph::coroutine< module_ast_spec > module_ast_impl(std::string input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_MODULE_AST_SPEC_HEADER_GUARD
