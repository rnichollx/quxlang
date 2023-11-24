#ifndef RPNX_GRAPH_SOLVER_HPP
#define RPNX_GRAPH_SOLVER_HPP

#include <cassert>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <coroutine>

#include "rpnx/error_explainer.hpp"

#include <__coroutine/coroutine_handle.h>

namespace rpnx
{
    // Resolver shall have
    // key_type
    // result_type
    // graph_type

    template < typename Graph, typename Resolver >
    class index;

    template < typename T >
    class result
    {
        std::optional< T > t;
        std::exception_ptr er;

      public:
        result(T t)
            : t(t)
        {
        }

        result()
        {
        }

        result(std::exception_ptr er)
            : er(er)
        {
        }

        T get() const
        {
            if (er)
                std::rethrow_exception(er);
            return t.value();
        }

        void set_value(T t)
        {
            this->er = nullptr;
            this->t = t;
        }

        void set_error(std::exception_ptr er)
        {
            this->t.reset();
            this->er = er;
        }

        bool has_value() const
        {
            return t.has_value();
        }

        bool has_error() const
        {
            return er != nullptr;
        }

        std::exception_ptr get_error() const
        {
            return er;
        }

        inline operator bool() const
        {
            return has_value() || has_error();
        }
    };

    template < typename Graph >
    class node_base
    {
      public:
        virtual ~node_base(){};


        virtual std::string question() const
        {
           return "?";
        }

      public:
        void refresh_dependency(node_base< Graph >* n)
        {
            if (n->resolved() && !n->has_error())
            {
                m_met_dependencies.insert(n);
                m_unmet_dependencies.erase(n);
                m_error_dependencies.erase(n);
            }
            else if (n->has_error())
            {
                m_error_dependencies.insert(n);
                m_met_dependencies.erase(n);
                m_unmet_dependencies.erase(n);
            }
            else
            {
                m_unmet_dependencies.insert(n);
                m_met_dependencies.erase(n);
                m_error_dependencies.erase(n);
            }
        }
        void add_dependency(node_base< Graph >* n)
        {

            if (m_met_dependencies.find(n) != m_met_dependencies.end())
            {
                return;
            }
            if (m_unmet_dependencies.find(n) != m_unmet_dependencies.end())
            {
                return;
            }
            if (m_error_dependencies.find(n) != m_error_dependencies.end())
            {
                return;
            }

            n->m_dependents.insert(this);

            if (!n->resolved())
            {
                m_unmet_dependencies.insert(n);
            }
            else if (n->has_error())
            {
                m_error_dependencies.insert(n);
            }
            else
            {
                m_met_dependencies.insert(n);
            }
        }

        void add_dependency(std::shared_ptr< node_base< Graph > > n)
        {
            add_dependency(n.get());
        }

        template < typename F >
        auto get_dependency(F f)
        {
            auto result = f();
            add_dependency(result);
            return result;
        }

        template < typename Resolver, typename Key >
        auto get_dep_value(index< Graph, Resolver >& index, Key key)
        {
            auto dep = get_dependency(
                [&]()
                {
                    return index.lookup(key);
                });
            if (!dep->resolved())
            {
                throw dep_unresolved_ex{};
            }
            return dep->get();
        }

        struct dep_unresolved_ex
        {
        };

        bool resolved() const
        {
            return has_error() || has_value();
        }

        virtual bool has_error() const = 0;
        virtual bool has_value() const = 0;
        virtual std::exception_ptr get_error() const = 0;
        virtual void set_error(std::exception_ptr) = 0;

        virtual void process(Graph* graph) = 0;
        bool has_unresolved_dependencies() const
        {
            return m_unmet_dependencies.size() > 0;
        }
        bool has_error_dependencies() const
        {
            return m_error_dependencies.size() > 0;
        }

        bool ready() const
        {
            return !has_unresolved_dependencies() && !resolved();
        }

        void update_dependents()
        {
            for (auto& d : m_dependents)
            {
                d->refresh_dependency(this);
            }
        }

        std::set< node_base< Graph >* > const& dependents() const
        {
            return m_dependents;
        }

        std::set< node_base< Graph >* > const& unmet_dependencies() const
        {
            return m_unmet_dependencies;
        }

        std::set< node_base< Graph >* > const& error_dependencies() const
        {
            return m_error_dependencies;
        }

        std::set< node_base< Graph >* > const& met_dependencies() const
        {
            return m_met_dependencies;
        }

      private:


        std::set< node_base< Graph >* > m_met_dependencies;
        std::set< node_base< Graph >* > m_unmet_dependencies;
        std::set< node_base< Graph >* > m_error_dependencies;

        std::set< node_base< Graph >* > m_dependents;
    };

    template < typename Graph, typename Result >
    class resolver_base : public virtual node_base< Graph >
    {
      public:
        using value_type = Result;
        using result_type = result< Result >;

        virtual ~resolver_base(){};



        Result get() const
        {
            return m_result.get();
        }

        std::exception_ptr get_error() const override
        {
            return m_result.get_error();
        }

      protected:
        void set_value(Result r)
        {
            m_result.set_value(r);
        }
      public:
        virtual void set_error(std::exception_ptr er) override
        {
            m_result.set_error(er);
        }

      public:
        virtual bool has_value() const override final
        {
            return m_result.has_value();
        }

        virtual bool has_error() const override final
        {
            return m_result.has_error();
        }

      private:
        result_type m_result;
    };

    template < typename Graph, typename Result >
    using output_ptr = std::shared_ptr< resolver_base< Graph, Result > >;

    template <typename Graph, typename Result>
    class resolver_coroutine
        : resolver_base<Graph, Result>
    {
    private:

        struct promise_type
        {
            resolver_coroutine * cr;

            auto get_return_value()
            {
                return resolver_coroutine{this};
            }

            auto initial_suspend()
            {
                return std::suspend_always{};
            }

            auto final_suspend()
            {
                if (!cr->has_error() && !cr->has_value() && cr->ready())
                {
                    throw std::runtime_error("Resolver did not set value or error");
                }
                return std::suspend_never{};
            }

            ~promise_type()
            {
                if (cr)
                {
                    cr->pr = nullptr;
                }
            }

            void unhandled_exception()
            {
                assert(cr);
                cr->set_error(std::current_exception());
            }

            void return_value(Result r)
            {
                assert(cr);
                cr->set_value(r);
            }

        };
        promise_type * pr{};
    public:
        resolver_coroutine(promise_type * pr)
            : pr(pr)
        {
            assert(pr != nullptr);
        }

        ~resolver_coroutine()
        {
            if (pr)
            {
                std::coroutine_handle<promise_type>::from_promise(*pr).destroy();
            }

            assert(pr == nullptr);
        }



    };

    template <typename Graph, typename Result>
    class resolver_base_awaitable
    {
    private:
        node_base<Graph> * m_node;
        std::shared_ptr< resolver_base<Graph, Result> > m_resolver;
    public:
        resolver_base_awaitable(node_base<Graph> * node, std::shared_ptr< resolver_base<Graph, Result> > resolver)
            : m_node(node), m_resolver(resolver)
        {
            m_node->add_dependency(m_resolver);
        }

        auto await_ready() noexcept
        {
            return m_resolver->ready();
        }

        auto await_suspend(std::coroutine_handle<>)
        {
            // This is a no-op because resolvers wait
            // for processors to resume stalled resolvables.
        }

        Result await_resume()
        {
            return m_resolver->get();
        }

    };

    template < typename Graph, typename Resolver >
    class index
    {

      public:
        using key_type = typename Resolver::key_type;
        using value_type = typename Resolver::value_type;
        using result_type = result< value_type >;
        using resolver_type = Resolver;
        using resolver_ptr = std::shared_ptr< resolver_type >;

        static_assert(std::is_base_of< resolver_base< Graph, value_type >, Resolver >::value, "Resolver must implement resolver_base");

        resolver_ptr lookup(key_type const& k)
        {
            if (m_resolvers.count(k) == 0)
            {
                m_resolvers[k] = std::make_shared< resolver_type >(k);
            }
            return m_resolvers[k];
        }

      private:
        std::map< key_type, resolver_ptr > m_resolvers;
    };

    template < typename Graph, typename Resolver >
    class singleton
    {
      public:
        using resolver_ptr = std::shared_ptr< Resolver >;

      private:
        resolver_ptr m_resolver;

      public:
        resolver_ptr lookup()
        {
            if (!m_resolver)
            {
                m_resolver = std::make_shared< Resolver >();
            }
            return m_resolver;
        }
    };

    template < typename Graph >
    class single_thread_graph_solver
    {
      public:
        inline void solve(Graph* graph, std::shared_ptr< node_base< Graph > > node)
        {
            solve(graph, node.get());
        }

        void solve(Graph* graph, node_base< Graph >* node)
        {
            std::set< node_base< Graph >* > nodes_to_process;
            nodes_to_process.insert(node);

            while (nodes_to_process.size() > 0)
            {
                node_base< Graph >* n = *nodes_to_process.begin();
                nodes_to_process.erase(n);
                if (n->resolved() || !n->ready())
                {
                    continue;
                }

                try
                {
                    n->process(graph);
                }
                catch (...)
                {
                    assert(!n->has_value());
                    n->set_error(std::current_exception());
                }
                assert(n->resolved() || n->has_unresolved_dependencies());
                if (n->has_error())
                {
                   std::cout << "Error in " << n->question() << std::endl;
                   std::cout << "Error: " << explain_error(*n) << std::endl;
                }

                if (n->resolved())
                {
                    auto dependents = n->dependents();
                    for (auto& d : dependents)
                    {
                        assert(!d->resolved());
                        d->refresh_dependency(n);
                        if (d->ready())
                        {
                            nodes_to_process.insert(d);
                        }
                    }
                }
                else
                {
                    auto requirements = n->unmet_dependencies();
                    for (auto& req : requirements)
                    {
                        if (req->ready())
                        {
                            nodes_to_process.insert(req);
                        }
                    }
                    assert(requirements.size() != 0);
                }
            }

            if (!node->resolved())
            {
                throw std::runtime_error("Could not resolve node, probably recursive dependency");
            }
        }
    };

} // namespace rpnx

#endif