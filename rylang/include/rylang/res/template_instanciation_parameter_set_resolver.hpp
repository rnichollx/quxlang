//
// Created by Ryan Nicholl on 12/18/23.
//

#ifndef TEMPLATE_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
#define TEMPLATE_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
#include <rpnx/resolver_utilities.hpp>
#include <rylang/compiler_fwd.hpp>
#include <rylang/data/temploid_instanciation_parameter_set.hpp>

namespace rylang
{
    class template_instanciation_parameter_set_resolver
        : public rpnx::co_resolver_base< compiler, temploid_instanciation_parameter_set, instanciation_reference >
    {
    public:
        template_instanciation_parameter_set_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual std::string question() const override;

        virtual auto co_process(compiler * c, input_type input) -> co_type override;
    };
}

#endif //TEMPLATE_INSTANCIATION_PARAMETER_SET_RESOLVER_HPP
