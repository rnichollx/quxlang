// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include <quxlang/data/compilation_result.hpp>
#include "quxlang/manipulators/merge_entity.hpp"
#include "quxlang/variant_utils.hpp"
#include <iostream>


void quxlang::merge_entity(ast2_symboid& destination, declaroid const& source)
{

    std::string kind = source.type().name();
    if (typeis< ast2_function_declaration >(source))
    {

        auto const& func = as< ast2_function_declaration >(source);

        if (typeis< std::monostate >(destination))
        {
            destination = functum{};
        }

        else if (!typeis< functum >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge function into non-function of the same name");
        }

        functum & destination_functum = as< functum >(destination);

        destination_functum.functions.push_back(func);
    }
    else if (typeis< ast2_class_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge class into non-class of the same name");
        }

        destination = as< ast2_class_declaration >(source);
    }
    else if (typeis< ast2_interface_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge interface into already existing entity");
        }

        destination = as< ast2_interface_declaration >(source);
    }
    else if (typeis< ast2_implementation_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge implementation into already existing entity");
        }

        destination = as< ast2_implementation_declaration >(source);
    }
    else if (typeis< ast2_enum_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge enum into already existing entity");
        }

        destination = as< ast2_enum_declaration >(source);
    }
    else if (typeis< ast2_flagset_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge flagset into already existing entity");
        }

        destination = as< ast2_flagset_declaration >(source);
    }
    else if (typeis< ast2_template_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_templex{};
        }
        if (!typeis< ast2_templex >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge template into non-template of the same name");
        }

        ast2_templex& destination_templex = as< ast2_templex >(destination);

        destination_templex.templates.push_back(as< ast2_template_declaration >(source));
    }
    else if (typeis< ast2_namespace_declaration >(source))
    {
        if (typeis< std::monostate >(destination))
        {
            destination = ast2_namespace_declaration{};
        }
        if (!typeis< ast2_namespace_declaration >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge namespace into non-namespace of the same name");
        }

        ast2_namespace_declaration& ns = as< ast2_namespace_declaration >(destination);

        for (auto& x : as< ast2_namespace_declaration >(source).declarations)
        {
            ns.declarations.push_back(x);
        }
    }
    else if (typeis< ast2_variable_declaration >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge variable into already existing entity");
        }

        destination = as< ast2_variable_declaration >(source);
    }
    else if (typeis< ast2_asm_procedure_declaration >(source))
    {

        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge asm procedure into already existing entity");
        }

        destination = as< ast2_asm_procedure_declaration >(source);
    }
    else if (typeis< ast2_static_test > (source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge static test into already existing entity");
        }

        destination = as < ast2_static_test >(source);
    }
    else if (typeis< ast2_option >(source))
    {
        if (!typeis< std::monostate >(destination))
        {
            throw quxlang::semantic_compilation_error("Cannot merge option into already existing entity");
        }

        destination = as< ast2_option >(source);
    }
    else
    {
        rpnx::unimplemented();
    }
}
