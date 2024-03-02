//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef QUXLANG_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD
#define QUXLANG_OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/canonical_type_reference.hpp"

namespace quxlang
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

} // namespace quxlang

#endif // OVERLOAD_SET_INSTANCIATE_WITH_RESOLVER_HEADER_GUARD
