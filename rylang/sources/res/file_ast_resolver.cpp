//
// Created by Ryan Nicholl on 7/20/23.
//
//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/compiler.hpp"


#include "rylang/collector.hpp"
#include <rylang/parsers/parse_file.hpp>

#include <exception>
//#include <iosfwd>
#include <iostream>

//#include <fstream>

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

    ast2_file_declaration v_file_ast;

    auto it = content.begin();
    v_file_ast = parsers::parse_file(it, content.end());

    v_file_ast.filename = input_filename;


    set_value(std::move(v_file_ast));
}