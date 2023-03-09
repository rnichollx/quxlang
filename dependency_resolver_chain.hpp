//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#include <atomic>
#include <functional>
#include <optional>
#include <vector>

namespace rs1031
{
    struct resolvable_base
    {
        std::size_t unmet_dependencies;
        std::vector< resolvable_base* > dependents;
        virtual void resolve() = 0;
        virtual ~resolvable_base() = default;
    };

    template < typename T >
    struct resolvable : resolvable_base
    {
        std::function< T() > resolver;
        std::optional< T > resolved_value;
        virtual void resolve() override
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a resolvable with unmet dependencies");
            }
            resolved_value = resolver();
            for (auto* d : dependents)
            {
                d->unmet_dependencies--;

            }
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
