// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/module_ast_spec.hpp>
#include <quxlang/macros.hpp>

#include "quxlang/manipulators/merge_entity.hpp"

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/ast2/ast2_module.hpp>
#include <quxlang/parsers/parse_file.hpp>

#include <exception>

rpnx::querygraph::coroutine< quxlang::module_ast_spec > quxlang::module_ast_impl(std::string input)
{

    auto srcs = co_await rpnx::querygraph::request< module_sources_query >(input);

    ast2_module_declaration result;

    for (auto& file : srcs.files)
    {
        ast2_file_declaration v_file_ast;

        auto content = file.second->contents;
        auto input_filename = input + "/" + file.first;

        auto it = content.begin();

        std::exception_ptr parse_error;
        std::string error_snippet;
        std::string error_message;
        try
        {
            v_file_ast = parsers::parse_file(it, content.end());
        }
        catch (std::exception& e)
        {
            auto distance_to_end = std::distance(it, content.end());

            std::string::iterator end_snippet_iter = it + std::min(distance_to_end, ptrdiff_t(100));

            std::string snippet(it, end_snippet_iter);

            error_snippet = std::move(snippet);
            error_message = e.what();
            parse_error = std::current_exception();
        }

        if (parse_error)
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                co_yield rpnx::querygraph::debug_message("At:  {}", error_snippet);
                co_yield rpnx::querygraph::debug_message("Error: {}", error_message);
            }

            std::rethrow_exception(parse_error);
        }

        // TODO: consider if we should keep v_file_ast.filename
        v_file_ast.filename = input_filename;


        // TODO: Check for duplicate imports
        for (auto import : v_file_ast.imports)
        {
            result.imports.insert(import);
        }

        for (auto decl : v_file_ast.declarations)
        {
            result.declarations.push_back(decl);
        }
    }

    co_return result;

} // namespace quxlang
