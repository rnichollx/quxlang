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
    class dependency_func_node;

    struct dependency_base : std::enable_shared_from_this< dependency_base >
    {
        template < typename T >
        friend class dependency_func_node;

      private:
        // TODO: Add multithreading support, design a threading model for resolvables.
        std::atomic< ssize_t > unmet_dependencies = 0;
        std::vector< std::weak_ptr< dependency_base > > dependents;

      public:
        void increment_dependencies()
        {
            unmet_dependencies++;
        }

        void decrease_dependencies(std::function< void(std::shared_ptr< dependency_base >) > emitter)
        {
            if (unmet_dependencies.fetch_sub(1) == 1)
            {
                emitter(shared_from_this());
            }
        }

        enum class state { resolved, unresolvable, maybe_resolvable };

        virtual state get_state() const = 0;

        bool is_maybe_resolvable() const
        {
            return get_state() == state::maybe_resolvable;
        }

        bool is_resolved() const
        {
            return get_state() == state::resolved;
        }

        bool is_unresolvable() const
        {
            return get_state() == state::unresolvable;
        }

        virtual bool try_resolve(std::function< void(std::shared_ptr< dependency_base >) >) = 0;
        virtual ~dependency_base() = default;

        void add_dependency(std::shared_ptr< dependency_base > d)
        {
            assert(d != nullptr);
            if (d->get_state() == state::resolved)
            {
                return;
            }

            d->dependents.push_back(shared_from_this());
            unmet_dependencies++;
        }
    };

    template < typename T >
    class dependency_func_node : public dependency_base
    {
      public:
        using resolver_function_type = std::function< bool(std::optional< T >&, std::shared_ptr< dependency_func_node< T > >) >;

      private:
        resolver_function_type resolver_functor;
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

        void set_resolver(resolver_function_type f)
        {
            assert(get_state() != state::resolved);
            resolver_functor = f;
        }

        virtual state get_state() const override final
        {
            if (resolved_value.has_value())
            {
                return state::resolved;
            }
            else if (unmet_dependencies != 0 || resolver_functor == nullptr)
            {
                return state::unresolvable;
            }
            else
            {
                assert(resolver_functor != nullptr);
                return state::maybe_resolvable;
            }
        }
        bool try_resolve(std::function< void(std::shared_ptr< dependency_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a dependency_func_node with unmet dependencies");
            }
            bool worked = resolver_functor(resolved_value, std::dynamic_pointer_cast< dependency_func_node< T > >(shared_from_this()));
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
                throw std::runtime_error("Attempted to get a value from a dependency_func_node that has not been resolved");
            }
            return resolved_value.value();
        }

        dependency_func_node(resolver_function_type f)
            : resolver_functor(f)
        {
            unmet_dependencies = 0;
        }
    };

    template <>
    struct dependency_func_node< void > : public dependency_base
    {
        std::vector< std::function< void() > > resolve_queue{};
        std::function< bool() > resolver_functor;
        bool m_resolved = false;

      public:
        dependency_func_node(std::function< bool() > f, std::vector< std::shared_ptr< dependency_base > > const& deps, int initial_dependencies = 0)
            : resolver_functor(std::move(f))
        {
            unmet_dependencies = initial_dependencies;
            for (auto& d : deps)
            {
                add_dependency(shared_from_this());
            }
        }

        bool try_resolve(std::function< void(std::shared_ptr< dependency_base >) > f) override final
        {
            if (unmet_dependencies != 0)
            {
                throw std::runtime_error("Attempted to resolve a dependency_func_node with unmet dependencies");
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

        virtual state get_state() const override final
        {
            if (m_resolved)
            {
                assert(unmet_dependencies == 0);
                return state::resolved;
            }
            else if (unmet_dependencies != 0)
            {
                return state::unresolvable;
            }
            else
            {
                assert(resolver_functor != nullptr);
                return state::maybe_resolvable;
            }
        }
    };

    template < typename T >
    using dep_func_ptr = std::shared_ptr< dependency_func_node< T > >;

    template < typename T >
    using dep_func_wk_ptr = std::weak_ptr< dependency_func_node< T > >;

    template < typename T >
    using dep_res_func = typename dependency_func_node< T >::resolver_function_type;

    template < typename T >
    auto make_dep_func(dep_res_func< T > f)
    {
        return std::make_shared< dependency_func_node< T > >(f);
    }
} // namespace rs1031

#endif // RPNX_RYANSCRIPT1031_DEPENDENCY_RESOLVER_CHAIN_HEADER
