//
// Created by Ryan Nicholl on 3/8/23.
//

#ifndef RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#define RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

/* Here is a software design pattern: There is a generic class of T that can be in 3 states, resolved, unresolvable, and maybe_resolvable.

The class is created with an argument function/functor to "resolve" a value. When an instance is resolved, a get() function can be called to get an instance of T. The class also has the ability to add
dependencies to other instances of the generic class that are not "resolved", and those instances do not need to be of the same T type.

When an instance transitions to the resolved state, a counter for "unresolved dependencies" of objects that depend on it is decreased, if the "unresolved_dependencies" counter becomes 0, the state
would transition from "unresolvable" to "maybe_resolvable".

When an object is in the maybe_resolvable state, the "try_resolve" operation either transitions the object to the "resolved" state returning true, or instead adds new dependencies and transitions it
to the "unresolvable" state, returning false.

When doing the "try_resolve" operation, the function returns a list of objects which transitioned from the "unresolvable" to "maybe_resolvable" state.

 */
namespace rs1031
{
    template < typename T >
    class resolvable;

    struct resolvable_base : std::enable_shared_from_this< resolvable_base >
    {
        template < typename T >
        friend class resolvable;

      private:
        // TODO: Add multithreading support, design a threading model for resolvables.
        std::atomic< ssize_t > unmet_dependencies = 0;
        std::vector< std::weak_ptr< resolvable_base > > dependents;

      public:
        void increment_dependencies()
        {
            unmet_dependencies++;
        }

        void decrease_dependencies(std::function< void(std::shared_ptr< resolvable_base >) > emitter)
        {
            if (unmet_dependencies.fetch_sub(1) == 1)
            {
                emitter(shared_from_this());
            }
        }

        bool is_resolvable() const
        {
            return unmet_dependencies == 0;
        }

        virtual bool try_resolve(std::function< void(std::shared_ptr< resolvable_base >) >) = 0;
        virtual ~resolvable_base() = default;

        void add_dependency(std::shared_ptr< resolvable_base > d)
        {
            d->dependents.push_back(shared_from_this());
            unmet_dependencies++;
        }
    };

    template < typename T >
    class resolvable : public resolvable_base
    {
        std::function< bool(std::optional< T >&) > resolver_functor;
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

        bool try_resolve(std::function< void(std::shared_ptr< resolvable_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a resolvable with unmet dependencies");
            }
            bool worked = resolver_functor(resolved_value);
            if (!worked)
            {
                return false;
            }
            resolver_functor = nullptr;
            for (auto d : dependents)
            {
                if (auto s = d.lock())
                {
                    s->decrease_dependencies(f);
                }
            }
            for (auto& f2 : resolve_queue)
            {
                f2(resolved_value.value());
            }
            resolve_queue.clear();
            return true;
        }

        T get() const
        {
            if (!resolved_value.has_value())
            {
                throw std::runtime_error("Attempted to get a value from a resolvable that has not been resolved");
            }
            return resolved_value.value();
        }

        resolvable(std::function< bool(std::optional< T >&) > f, std::vector< std::shared_ptr< resolvable_base > > const& deps, int initial_dependencies = 0)
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
        std::function< bool() > resolver_functor;

      public:
        resolvable(std::function< bool() > f, std::vector< std::shared_ptr< resolvable_base > > const& deps, int initial_dependencies = 0)
            : resolver_functor(std::move(f))
        {
            unmet_dependencies = initial_dependencies;
            for (auto& d : deps)
            {
                add_dependency(shared_from_this());
            }
        }

        bool try_resolve(std::function< void(std::shared_ptr< resolvable_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a resolvable with unmet dependencies");
            }
            bool worked = resolver_functor();
            if (!worked)
            {
                return false;
            }
            resolver_functor = nullptr;
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
            return true;
        }
    };
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
