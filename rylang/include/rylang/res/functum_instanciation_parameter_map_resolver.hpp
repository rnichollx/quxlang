//
// Created by Ryan Nicholl on 12/19/23.
//

#ifndef FUNCTUM_INSTANCIATION_PARAMETER_MAP_RESOLVER_HPP
#define FUNCTUM_INSTANCIATION_PARAMETER_MAP_RESOLVER_HPP
#include <rpnx/resolver_utilities.hpp>
#include <rylang/data/qualified_symbol_reference.hpp>
#include <rylang/data/temploid_instanciation_parameter_set.hpp>

namespace rylang
{
    class compiler;
    class functum_instanciation_parameter_map_resolver
        : public rpnx::co_resolver_base<compiler, temploid_instanciation_parameter_set, instanciation_reference>
    {
    public:
        functum_instanciation_parameter_map_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual auto co_process(compiler * c, input_type input) -> co_type override;


    };

}

#endif //FUNCTUM_INSTANCIATION_PARAMETER_MAP_RESOLVER_HPP
