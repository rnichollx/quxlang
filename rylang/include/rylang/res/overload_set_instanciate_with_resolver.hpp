//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef RYLANG_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HPP
#define RYLANG_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HPP

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    class overload_set_instanciate_with_resolver : public rpnx::co_resolver_base< compiler, std::optional< call_parameter_information >, std::pair< call_parameter_information, call_parameter_information > >
    {

      public:
        overload_set_instanciate_with_resolver(input_type input)
            : co_resolver_base(input)
        {
            for (auto & arg: input.second.argument_types)
            {
                assert(!is_template(arg));
            }
        }

        virtual rpnx::resolver_coroutine< compiler, std::optional< call_parameter_information > > co_process(compiler* c, input_type input) override;
    };

} // namespace rylang

#endif // OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HPP
