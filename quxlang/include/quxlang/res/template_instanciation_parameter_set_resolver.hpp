//
// Created by Ryan Nicholl on 12/18/23.
//

#ifndef QUXLANG_RES_TEMPLATE_INSTANCIATION_PARAMETER_SET_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_TEMPLATE_INSTANCIATION_PARAMETER_SET_RESOLVER_HEADER_GUARD
#include <rpnx/resolver_utilities.hpp>
#include <quxlang/compiler_fwd.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

namespace quxlang
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
