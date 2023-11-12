//
// Created by Ryan Nicholl on 10/28/23.
//

#include "rylang/res/overload_set_is_callable_with_resolver.hpp"
#include "rylang/compiler.hpp"
#include <vector>

void rylang::overload_set_is_callable_with_resolver::process(compiler* c)
{
    // TODO: support default values for arguments
    if (os.argument_types.size() != args.argument_types.size())
    {
        set_value(false);
        return;
    }

    std::vector< rylang::compiler::out< bool > > convertibles_dp;

    for (int i = 0; i < os.argument_types.size(); i++)
    {
        auto convertible_dp = get_dependency(
            [&]
            {
                return c->lk_canonical_type_is_implicitly_convertible_to(std::make_pair(args.argument_types[i], os.argument_types[i]));
            });

        if (!ready())
            return;

        bool convertible = convertible_dp->get();

        if (!convertible)
        {
            set_value(false);
            return;
        }
    }

    set_value(true);
}
