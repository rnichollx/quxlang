//
// Created by Ryan Nicholl on 12/18/23.
//

#ifndef FUNCTUM_INSTANCIATION_RESOLVER_HPP
#define FUNCTUM_INSTANCIATION_RESOLVER_HPP
#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/compiler.hpp>

namespace rylang
{
    class functum_instanciation_resolver
        : public rpnx::co_resolver_base< compiler, ast2_function_declaration, instanciation_reference >
    {
    public:
        functum_instanciation_resolver(input_type input)
            : co_resolver_base(input)
        {}

        virtual auto co_process(compiler * c, input_type input) -> co_type override;
    };
}

#endif //FUNCTUM_INSTANCIATION_RESOLVER_HPP
