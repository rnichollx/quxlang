//
// Created by Ryan Nicholl on 10/3/2024.
//

#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/data/type_symbol.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{

    // Parameters of is the "parameters" of a function or template,
    // as they are declared.
    QUX_CO_RESOLVER(parameters_of, selection_reference, parameters);


    struct parameters_with_context
    {
        type_symbol context;
        parameters params;

        RPNX_MEMBER_METADATA(parameters_with_context, context, params);
    };

    // Formal paramters differ from parameters in that they are decontextualized, but not instanciated,
    // so for example, baz::foo(%a bar) would have a nominal parameters of %a bar, but if "bar" is a subclass
    // of "baz", then the formal parameter type would be %a [module]::baz::bar.
    // Note that temploidic types like T(...) are not resolved here and are left as is.
    QUX_CO_RESOLVER(formal_parameters_of, selection_reference, parameters);
}

#endif //PARAMETERS_HPP
