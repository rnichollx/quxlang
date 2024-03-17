//
// Created by Ryan Nicholl on 7/20/23.
//
//
// Created by Ryan Nicholl on 7/20/23.
//

#include "quxlang/compiler.hpp"


#include <quxlang/parsers/parse_file.hpp>

#include <exception>
//#include <iosfwd>
#include <iostream>

//#include <fstream>

void quxlang::file_ast_resolver::process(compiler* c)
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



    ast2_file_declaration v_file_ast;

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

    v_file_ast.filename = input_filename;


    set_value(std::move(v_file_ast));
}