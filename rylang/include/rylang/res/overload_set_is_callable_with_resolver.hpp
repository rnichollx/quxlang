//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RYLANG_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
#define RYLANG_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/call_parameter_information.hpp"
#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    class overload_set_is_callable_with_resolver : public rpnx::co_resolver_base< compiler, bool, std::pair< call_parameter_information, call_parameter_information > >
    {

      public:

        overload_set_is_callable_with_resolver(input_type input)
            : co_resolver_base(input)
        {
            for (auto & arg: input.second.argument_types)
            {
                if (is_template(arg))
                {
                    throw std::logic_error("this shouldn't be possible");
                }
            }
        }

        virtual rpnx::resolver_coroutine<compiler, bool> co_process(compiler* c, input_type input);

        virtual std::string question() const override
        {
            return "Is " + to_string(input_val.first) + " callable with " + to_string(input_val.second) + "?";
        }

        virtual std::string answer() const override
        {
            return has_value() ? (get() ? "Yes" : "No") : "erorr";
        }
    };


} // namespace rylang

#endif // RYLANG_OVERLOAD_SET_IS_CALLABLE_WITH_RESOLVER_HEADER_GUARD
