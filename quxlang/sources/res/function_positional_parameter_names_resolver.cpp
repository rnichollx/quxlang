//
// Created by Ryan Nicholl on 4/20/24.
//

#include <quxlang/res/function_positional_parameter_names_resolver.hpp>

#include <quxlang/compiler.hpp>
#include <quxlang/macros.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_positional_parameter_names)
{
    QUX_CO_GETDEP(ast, entity_ast_from_canonical_chain, (input_val));

    if (!typeis< functum >(ast))
    {

        throw std::logic_error("Expected functum");
    }

    functum const& f = as< functum >(ast);

    for (auto const& func : f.functions)
    {
        auto head = func.first;
        if (head == input_val.header)
        {
            std::vector< std::string > strs;

            // TODO: Implement this
            //  for (auto const& param : func.first.parameters.)
        }
    }

    rpnx::unimplemented();
}
