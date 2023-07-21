#ifndef RPNX_GRAPH_SOLVER_HPP
#define RPNX_GRAPH_SOLVER_HPP

#include <cassert>
#include <map>
#include <optional>
#include <set>
#include <type_traits>
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

        template <typename Resolver, typename Key>
        auto get_dep_value(index<Graph, Resolver> & index, Key key)
        {
            auto dep = get_dependency([&](){ return index.lookup(key);});
            if (!dep->resolved())
            {
                throw dep_unresolved_ex{};
            }
            return dep->get();
        }

        struct dep_unresolved_ex {};

        bool resolved() const
        {
            return has_error() || has_value();
        }

        virtual bool has_error() const = 0;
        virtual bool has_value() const = 0;

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
            return !has_unresolved_dependencies() && !has_error_dependencies() && !resolved();
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
    class output_base : public virtual node_base< Graph >
    {
      public:
        using result_type = result< Result >;

        virtual ~output_base(){};

        Result get() const
        {
            return m_result.get();
        }

        Result get_error() const
        {
            return m_result.get_error();
        }

      protected:
        void set_value(Result r)
        {
            m_result.set_value(r);
        }

        void set_error(std::exception_ptr er)
        {
            m_result.set_error(er);
        }

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
    using output_ptr = std::shared_ptr< output_base< Graph, Result > >;

    template < typename Graph, typename Func >
    class functional_resolver;

    template < typename Graph, typename R, typename... Ts >
    class functional_resolver< Graph, R(Ts...) >
    {
        using key_type = std::tuple< Ts... >;
        using value_type = R;
        using result_type = result< value_type >;

      private:
        std::function< result_type(functional_resolver< Graph, R(Ts...) >*, Graph*, Ts...) > m_func;

      public:
        template < typename F >
        functional_resolver(F f)
            : m_func(f)
        {
        }

        virtual void process(Graph* graph)
        {
            m_func(*this, graph, std::get< Ts >()...);
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

        static_assert(std::is_base_of< output_base< Graph, value_type >, Resolver >::value, "Resolver must implement resolver_base");

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

                n->process(graph);
                assert(n->resolved() || n->has_unresolved_dependencies());

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
                        nodes_to_process.insert(req);
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