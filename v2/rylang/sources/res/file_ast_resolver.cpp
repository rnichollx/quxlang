//
// Created by Ryan Nicholl on 7/20/23.
//
//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/res/file_ast_resolver.hpp"
#include "rylang/collector.hpp"
#include "rylang/compiler.hpp"
#include <exception>
#include <fstream>


void rylang::file_ast_resolver::process(compiler* c)
{
    // auto input_file_name = m_input_filename;
    auto content_ptr = get_dependency(
        [&]
        {
            return c->file_contents(input_filename);
        });
    if (!ready())
        return;

    auto content = content_ptr->get();

    collector col;

    file_ast v_file_ast;

    col.emit_class = [&](std::string name, class_ast ast)
    {
        v_file_ast.classes[name] = std::move(ast);
    };
}