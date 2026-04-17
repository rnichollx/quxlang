// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_ast_spec.hpp>

#include <vector>

rpnx::querygraph::coroutine< quxlang::module_ast_spec > quxlang::module_ast_impl(std::string input)
{

    auto const& srcs = co_await rpnx::querygraph::request< module_sources_query >(input);
    auto source_module_name = co_await rpnx::querygraph::request< module_source_name_query >(input);

    ast2_module_declaration result;
    std::vector< rpnx::querygraph::request< parse_file_query > > parse_requests;
    parse_requests.reserve(srcs.files.size());

    for (auto const& [relative_path, file] : srcs.files)
    {
        (void)file;
        parse_requests.emplace_back(source_file_name{.source_module = source_module_name, .relative_path = relative_path});
        co_yield rpnx::querygraph::dependency(parse_requests.back());
    }

    for (auto const& parse_request : parse_requests)
    {
        auto const& v_file_ast = co_await parse_request;

        // TODO: Check for duplicate imports
        for (auto const& import : v_file_ast.imports)
        {
            result.imports.insert(import);
        }

        for (auto const& decl : v_file_ast.declarations)
        {
            result.declarations.push_back(decl);
        }
    }

    co_return result;

} // namespace quxlang
