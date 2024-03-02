//
// Created by Ryan Nicholl on 12/20/23.
//

#ifndef TEMPLOID_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
#define TEMPLOID_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
#include <rpnx/resolver_utilities.hpp>
#include <quxlang/compiler_fwd.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

namespace quxlang
{
    class temploid_instanciation_parameter_set_resolver
        : public rpnx::co_resolver_base<compiler, temploid_instanciation_parameter_set, instanciation_reference>
    {
    public:
        temploid_instanciation_parameter_set_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual auto co_process(compiler * c, input_type) -> co_type override;
    };

}

#endif //TEMPLOID_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
