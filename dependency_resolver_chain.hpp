//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#include <atomic>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace rs1031
{
    struct resolvable_base : std::enable_shared_from_this< resolvable_base >
    {
      protected:
        int unmet_dependencies = 0;
        std::vector< std::weak_ptr< resolvable_base > > dependents;

      public:
        void increment_dependencies()
        {
            unmet_dependencies++;
        }

        void decrease_dependencies(std::function< void(std::shared_ptr< resolvable_base >) > emitter)
        {
            unmet_dependencies--;
            if (unmet_dependencies == 0)
            {
                emitter(shared_from_this());
            }
        }

        bool is_resolvable() const
        {
            return unmet_dependencies == 0;
        }

        virtual void resolve(std::function< void(std::shared_ptr< resolvable_base >) >) = 0;
        virtual ~resolvable_base() = default;

      protected:
        void add_dependency(std::shared_ptr< resolvable_base > d)
        {
            d->dependents.push_back(shared_from_this());
            unmet_dependencies++;
        }
    };

    template < typename T >
    class resolvable : public resolvable_base
    {
        std::function< T() > resolver_functor;
        std::optional< T > resolved_value;
        std::vector< std::function< void(T) > > resolve_queue;

      public:
        void on_resolve(std::function< void(T) > f)
        {
            if (resolved_value.has_value())
            {
                f(resolved_value.value());
            }
            else
            {
                resolve_queue.push_back(f);
            }
        }

        void resolve(std::function< void(std::shared_ptr< resolvable_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a resolvable with unmet dependencies");
            }
            resolved_value = resolver_functor();
            resolver_functor = nullptr;
            for (auto d : dependents)
            {
                if (auto s = d.lock())
                {
                    s->decrease_dependencies(f);
                }
            }
            for (auto& f : resolve_queue)
            {
                f(resolved_value.value());
            }
            resolve_queue.clear();
        }

        T get() const
        {
            if (!resolved_value.has_value())
            {
                throw std::runtime_error("Attempted to get a value from a resolvable that has not been resolved");
            }
            return resolved_value.value();
        }

        resolvable(std::function< T() > f, std::vector< std::shared_ptr< resolvable_base > > const& deps, int initial_dependencies = 0)
            : resolver_functor(f)
        {
            unmet_dependencies = initial_dependencies;
            for (auto& d : deps)
            {
                add_dependency(d);
            }
        }
    };

    template <>
    struct resolvable< void > : public resolvable_base
    {
        std::vector< std::function< void() > > resolve_queue{};
        std::function< void() > resolver_functor;

      public:
        resolvable(std::function< void() > f, std::vector< std::shared_ptr< resolvable_base > > const& deps, int initial_dependencies = 0)
            : resolver_functor(std::move(f))
        {
            unmet_dependencies = initial_dependencies;
            for (auto& d : deps)
            {
                add_dependency(shared_from_this());
            }
        }

        void resolve(std::function< void(std::shared_ptr< resolvable_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a resolvable with unmet dependencies");
            }
            resolver_functor();
            resolver_functor = std::function< void() >{};
            for (auto d : dependents)
            {
                if (auto dp = d.lock())
                {
                    dp->decrease_dependencies(f);
                }
            }
            for (auto& f : resolve_queue)
            {
                f();
            }
            resolve_queue.clear();
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
