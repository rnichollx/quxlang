#ifndef RPNX_GRAPH_SOLVER_HPP
#define RPNX_GRAPH_SOLVER_HPP

#include <cassert>
#include <coroutine>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>

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
            return !has_unresolved_dependencies() && !resolved() && busy_coroutines() == 0;
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

        std::size_t busy_coroutines() const
        {
            return m_busy_coroutines;
        }

        void coroutine_suspend(std::coroutine_handle<> handle)
        {
            m_waiting_coroutines.push_back(handle);
        }

        std::vector< std::coroutine_handle<> > take_waiting_coroutines()
        {
            std::vector< std::coroutine_handle<> > result = std::move(m_waiting_coroutines);

            m_waiting_coroutines.clear();

            return result;
        }

        void declare_coroutine_started()
        {
            m_busy_coroutines++;
        }

        void declare_coroutine_finished()
        {
            assert(m_busy_coroutines > 0);
            m_busy_coroutines--;
        }

      private:
        std::set< node_base< Graph >* > m_met_dependencies;
        std::set< node_base< Graph >* > m_unmet_dependencies;
        std::set< node_base< Graph >* > m_error_dependencies;
        std::set< node_base< Graph >* > m_dependents;
        std::size_t m_busy_coroutines;
        std::vector< std::coroutine_handle<> > m_waiting_coroutines;
    };

    template < typename Graph, typename Result >
    class resolver_coroutine;

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

        bool await_ready() const
        {
            return has_value() || has_error();
        }

        Result await_resume()
        {
            return get();
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< T > ch)
        {
            auto& pr = ch.promise();
            pr.cr->add_dependency(this);
        }

      private:
        result_type m_result;
    };

    template < typename Graph, typename Result >
    using output_ptr = std::shared_ptr< resolver_base< Graph, Result > >;

    // This class implements a general purpose coroutine that can be
    // used with a processor, but which is not itself a resolver.
    // It is expected to be called from within a resolver.
    template < typename Graph, typename Result >
    class general_coroutine
    {
        class promise_type
        {
          private:
            general_coroutine* cr = nullptr;

          public:

            ~promise_type()
            {
                cr->pr_ptr = nullptr;
            }
            auto get_return_object()
            {
                return general_coroutine{this};
            }

            auto initial_suspend()
            {
                return std::suspend_always{};
            }

            void unhandled_exception()
            {
                assert(cr != nullptr);
                cr->m_result.set_error(std::current_exception());
            }

            auto final_suspend() noexcept
            {
                assert(cr->m_result);
                return std::suspend_never{};
            }

            void return_value(Result r)
            {
                assert(cr != nullptr);
                assert(!cr->m_result);
                cr->m_result.set_value(r);
            }

            void return_void()
            {
                assert(cr != nullptr);
                assert(!cr->m_result);
                // TODO: Handle result != void type
                cr->m_result.set_value();
            }
        };

        promise_type* pr_ptr = nullptr;
        node_base< Graph >* ptr_waiter = nullptr;
        result< Result > m_result;
        std::coroutine_handle<> waiter_coroutine;

      private:
        general_coroutine(promise_type* arg_ptr_pr)
            : pr_ptr(arg_ptr_pr)
        {
            pr_ptr->cr = this;
        }

      public:
        general_coroutine()
        {
        }

        bool await_ready() noexcept
        {
            return m_result;
        }

        template < typename Result2 >
        void await_suspend(std::coroutine_handle< typename resolver_coroutine< Graph, Result2 >::promise_type > waiter_handle)
        {
            auto& waiter_promise = waiter_handle.promise();
            ptr_waiter = waiter_promise.cr;
            ptr_waiter->declare_coroutine_started();
        }

        auto await_resume()
        {
            return m_result.get();
        }
    };

    template < typename Graph, typename Result >
    class resolver_coroutine : public resolver_base< Graph, Result >
    {
      public:
        struct promise_type
        {
            resolver_coroutine* cr;

            auto get_return_object()
            {
                return resolver_coroutine{this};
            }

            auto initial_suspend()
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                if (!cr->has_error() && !cr->has_value() && cr->ready())
                {
                    try
                    {
                        throw std::logic_error("Resolver did not set value or error");
                    }
                    catch (...)
                    {
                        cr->set_error(std::current_exception());
                    }
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
        promise_type* pr{};
        bool movable = true;

      public:
        resolver_coroutine(promise_type* pr)
            : pr(pr)
        {
            assert(pr != nullptr);
        }

        resolver_coroutine(resolver_coroutine&& other)
        {
            assert(this->dependents().empty());
            if (!movable)
            {
                throw std::logic_error("Cannot move already started coroutine");
            }
            pr = other.pr;
            other.pr = nullptr;
            if (pr)
            {
                pr->cr = this;
            }
        }

        ~resolver_coroutine()
        {
            if (pr)
            {
                std::coroutine_handle< promise_type >::from_promise(*pr).destroy();
            }

            assert(pr == nullptr);
        }

        void process(Graph* g)
        {
            assert(this->ready());
            assert(pr != nullptr);
            movable = false;
            std::coroutine_handle< promise_type >::from_promise(*pr).resume();
            assert(this->has_value() || this->has_error() || !this->ready());
        }
    };

    template < typename Graph, typename Result, typename Question, typename... Args >
    class co_index
    {
      public:
        using input_type = std::tuple< Args... >;
        using result_type = result< Result >;
        using output_ptr = std::shared_ptr< resolver_coroutine< Graph, Result > >;

        template < typename... Ts >
        output_ptr lookup(Graph* g, Ts&&... ts)
        {
            input_type lookup_tuple(ts...);
            if (m_resolvers.count(lookup_tuple) == 0)
            {
                m_resolvers[lookup_tuple] = std::make_shared< resolver_coroutine< Graph, Result > >(Question::ask(g, ts...));
            }
            return m_resolvers[lookup_tuple];
        }

      private:
        std::map< input_type, output_ptr > m_resolvers;
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

            std::vector< std::coroutine_handle<> > coroutines_to_process;
            nodes_to_process.insert(node);

            while (nodes_to_process.size() > 0 || coroutines_to_process.size() > 0)
            {
                if (coroutines_to_process.size() != 0)
                {
                    std::coroutine_handle<> ch = coroutines_to_process.back();
                    coroutines_to_process.pop_back();
                    ch.resume();
                    continue;
                }

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
                assert(n->resolved() || n->has_unresolved_dependencies() || n->busy_coroutines() != 0);
                if (n->has_error())
                {
                    std::cout << "Error in " << n->question() << std::endl;
                    std::cout << "Error: " << explain_error(*n) << std::endl;
                }

                if (n->resolved())
                {
                    auto dependents = n->dependents();
                    auto coroutines = n->take_waiting_coroutines();
                    coroutines_to_process.insert(coroutines_to_process.end(), coroutines.begin(), coroutines.end());
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