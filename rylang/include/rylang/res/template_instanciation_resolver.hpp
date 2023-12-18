//
// Created by Ryan Nicholl on 12/17/23.
//

#ifndef TEMPLATE_INSTANCIATION_RESOLVER_HPP
#define TEMPLATE_INSTANCIATION_RESOLVER_HPP
#include <rpnx/resolver_utilities.hpp>
#include <rylang/ast2/ast2_entity.hpp>

namespace rylang
{
    class compiler;
}
namespace rylang
{

    class template_instanciation_resolver
        : public rpnx::co_resolver_base<compiler, ast2_map_entity, instanciation_reference>
    {
    public:
        template_instanciation_resolver(input_type input)
            : co_resolver_base(input)
        {
        }

        virtual auto co_process(compiler * c, input_type input) -> co_type override;

    };
}

#endif //TEMPLATE_INSTANCIATION_RESOLVER_HPP
