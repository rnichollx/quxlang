//
// Created by Ryan Nicholl on 7/20/23.
//

#include "quxlang/res/file_content_resolver.hpp"
#include <exception>
#include <fstream>

void quxlang::file_content_resolver::process(compiler* c)
{
    std::string output;
    std::ifstream input(input_filename);

    auto begin = std::istreambuf_iterator< char >(input);
    auto end = std::istreambuf_iterator< char >();

    output = std::string(begin, end);

    if (!input)
    {
        try
        {
            throw std::runtime_error("Could not open file " + input_filename);
        }
        catch (...)
        {
            set_error(std::current_exception());
        }
    }
    else
    {
        set_value(output);
    }
}