//
// Created by Ryan Nicholl on 9/20/23.
//

//
// Created by Ryan Nicholl on 9/11/23.
//

#include "quxlang/compiler.hpp"
#include "quxlang/manipulators/merge_entity.hpp"
#include "quxlang/res/module_ast_precursor1_resolver.hpp"

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/ast2/ast2_module.hpp>
#include <quxlang/parsers/parse_file.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(module_ast)
{

    QUX_CO_GETDEP(srcs, module_sources, (input_val));

    ast2_module_declaration result;

    for (auto& file : srcs.files)
    {
        ast2_file_declaration v_file_ast;

        auto content = file.second->contents;
        auto input_filename = input_val + "/" + file.first;

        auto it = content.begin();

        try
        {
            v_file_ast = parsers::parse_file(it, content.end());
        }
        catch (std::exception& e)
        {
            auto distance_to_end = std::distance(it, content.end());

            std::string::iterator end_snippet_iter = it + std::min(distance_to_end, ptrdiff_t(100));

            std::string snippet(it, end_snippet_iter);

            std::cout << "At:  " << snippet << std::endl;
            std::cout << "Error: " << e.what() << std::endl;

            throw;
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

    QUX_CO_ANSWER(result);

} // namespace quxlang