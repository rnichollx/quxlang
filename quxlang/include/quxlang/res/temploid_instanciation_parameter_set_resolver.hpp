// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TEMPLOID_INSTANCIATION_PARAMETER_SET_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_TEMPLOID_INSTANCIATION_PARAMETER_SET_RESOLVER_HEADER_GUARD
#include <rpnx/resolver_utilities.hpp>
#include <quxlang/compiler_fwd.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

namespace quxlang
{
    class temploid_instanciation_parameter_set_resolver
        : public rpnx::co_resolver_base<compiler, temploid_instanciation_parameter_set, initialization_reference>
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
