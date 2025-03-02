// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#ifndef RPNX_GRAPH_SOLVER_HPP
#define RPNX_GRAPH_SOLVER_HPP

#include <cassert>
#include <coroutine>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <type_traits>

#include "debug.hpp"
#include "rpnx/error_explainer.hpp"
#include "serializer.hpp"
#include <boost/core/demangle.hpp>
#include <concepts>
#include <memory>
#include <rpnx/result.hpp>
#include <sstream>

namespace rpnx
{
    template < typename T >
    struct resolver_traits;

    struct resolver_traits_string_visitor
    {
        template < typename T2 >
        std::string operator()(T2 const& v)
        {
            return rpnx::resolver_traits< T2 >::stringify(v);
        }
    };

    template < typename T >
    struct resolver_traits
    {

        static std::string default_stringify(bool v, int)
        {
            return v ? "true" : "false";
        }

        static std::string default_stringify(std::integral auto v, int)
        {
            std::stringstream ss;
            ss << v;
            return ss.str();
        }

        template < typename T2 >
        static std::string default_stringify(std::optional< T2 > const& v, int)
        {
            if (!v.has_value())
            {
                return "nullopt";
            }
            else
            {
                return rpnx::resolver_traits< T2 >::stringify(v.value());
            }
        }

        template < typename T2 >
        static std::string default_stringify(std::set< T2 > const& v, int)
        {
            std::stringstream ss;
            ss << "{";
            bool first = true;
            for (auto& e : v)
            {
                if (!first)
                {
                    ss << ", ";
                }
                ss << resolver_traits< T2 >::stringify(e);
                first = false;
            }
            ss << "}";
            return ss.str();
        }

        template < typename T2 >
        static std::string default_stringify(std::vector< T2 > const& v, int)
        {
            std::stringstream ss;
            ss << "{";
            bool first = true;
            for (auto& e : v)
            {
                if (!first)
                {
                    ss << ", ";
                }
                ss << resolver_traits< T2 >::stringify(e);
                first = false;
            }
            ss << "}";
            return ss.str();
        }

        template < typename... Ts >
        static std::string default_stringify(std::tuple< Ts... > const& input)
        {
            std::stringstream ss;
            ss << "{";
            std::apply(
                [&](auto&&... args)
                {
                    bool first = true;
                    ((first = (ss << (first ? "" : ", ") << resolver_traits< decltype(args) >::stringify(args))), ...);
                },
                input);
            ss << "}";
            return ss.str();
        }

        template < typename T2, typename T3 >
        static std::string default_stringify(std::pair< T2, T3 > const& v, int)
        {
            std::stringstream ss;
            ss << "{";
            ss << resolver_traits< T2 >::stringify(v.first);
            ss << ", ";
            ss << resolver_traits< T3 >::stringify(v.second);
            ss << "}";
            return ss.str();
        }

        template < typename T2, typename T3 >
        static std::string default_stringify(std::map< T2, T3 > const& v, int)
        {
            std::stringstream ss;
            ss << "{";
            bool first = true;
            for (auto& e : v)
            {
                if (!first)
                {
                    ss << ", ";
                }
                ss << resolver_traits< T2 >::stringify(e.first);
                ss << ": " << resolver_traits< T3 >::stringify(e.second);
                first = false;
            }
            ss << "}";
            return ss.str();
        }

        template < typename T2, typename T3 >
        static std::string default_stringify(std::string const& str, int)
        {
            return str;
        }

        template < typename T2 >
        static std::string default_stringify(T2&&, ...)
        {
            return std::string("?(") + boost::core::demangle(typeid(T).name()) + ")";
        }

        static std::string stringify(T const& t)
        {
            return default_stringify(t, 0);
        };
    };

    // Resolver shall have
    // key_type
    // result_type
    // graph_type

    template < typename Graph >
    struct coroutine_callback;

    template < typename Graph >
    class node_base;

    template < typename Graph >
    struct coroutine_exec_result
    {
        std::vector< node_base< Graph >* > ready_nodes;
        std::set< node_base< Graph >* > kickoff_nodes;
        std::vector< std::unique_ptr< coroutine_callback< Graph > > > ready_callbacks;
    };

    template < typename Graph >
    struct coroutine_callback
    {
        std::function< coroutine_exec_result< Graph >() > callback;
    };

    template < typename Graph, typename Resolver >
    class index;

    template < typename Graph >
    class single_thread_graph_solver;

    template < typename Graph >
    class node_base
    {
        friend class single_thread_graph_solver< Graph >;

        std::stringstream m_outstr;

      public:
        std::stringstream& out()
        {
            return m_outstr;
        }

      public:
        node_base< Graph >* m_attached_to = nullptr;
        node_base< Graph >* m_attached_by = nullptr;

      public:
        virtual ~node_base() {};

        virtual std::string question() const
        {
            if (m_attached_to)
            {
                return m_attached_to->question();
            }
            return typeid(*this).name();
        }

        virtual std::string answer() const
        {
            assert(resolved());
            if (m_attached_to)
            {
                return m_attached_to->answer();
            }
            if (has_value())
            {
                return "has_value";
            }
            else if (has_error())
            {
                try
                {
                    std::rethrow_exception(get_error());
                }
                catch (std::exception& e)
                {
                    return e.what();
                }
                catch (...)
                {
                    return "Unknown error";
                }
            }

            // assert(false);
            throw std::logic_error("unreachable");
        }

        std::string debug_line() const
        {
            std::stringstream ss;

            ss << question() << " => ";

            if (has_value())
            {
                ss << answer();
            }

            else if (has_error())
            {
                try
                {
                    auto er = get_error();

                    std::rethrow_exception(er);
                }
                catch (std::exception const& er)
                {
                    ss << er.what();
                }
                catch (...)
                {
                    ss << "error";
                }
                // TODO: Print error
            }

            else
            {
                ss << "unresolved";
            }

            return ss.str();
        }

        std::string debug() const
        {
            std::stringstream ss;

            ss << debug_line() << std::endl;

            // ss << "Inputs:" << std::endl;
            for (auto& d : met_dependencies())
            {
                ss << "    " << d->debug_line() << std::endl;
            }

            return ss.str();
        }

        std::string debug_old()
        {
            auto unmet_v = unmet_dependencies();
            if (unmet_v.size() == 1 && (*unmet_v.begin())->m_attached_to == this)
            {
                return (*unmet_v.begin())->debug();
            }
            std::stringstream ss;

            ss << "Q: " << question() << "\n";
            if (resolved())
            {
                std::string a = answer();
                if (a.size() > 150)
                {
                    a = a.substr(0, 150) + "...";
                }
                ss << "A: " << a << "\n";
            }
            else
            {
                ss << "A: unresolved\n";
            }

            ss << "Met dependencies:\n";
            for (auto& d : met_dependencies())
            {
                ss << "  " << d->question() << ", " << d->answer() << "\n";
            }

            ss << "Unmet dependencies:\n";

            for (auto& d : unmet_dependencies())
            {
                ss << "  " << d->question() << "\n";
            }

            ss << "Asked by:\n";
            if (m_attached_to)
            {
                for (auto& d : m_attached_to->dependents())
                {
                    ss << "  " << d->question() << "\n";
                }
            }
            else
            {
                for (auto& d : dependents())
                {
                    if (d != m_attached_to)
                    {
                        ss << "  " << d->question() << "\n";
                    }
                }
            }

            return ss.str();
        }

        std::string debug_recursive()
        {
            auto unmet_v = unmet_dependencies();
            if (unmet_v.size() == 1 && (*unmet_v.begin())->m_attached_to == this)
            {
                return (*unmet_v.begin())->debug_recursive();
            }
            std::stringstream ss;

            ss << "Q: " << question() << "\n";
            if (resolved())
            {
                std::string a = answer();
                if (a.size() > 150)
                {
                    a = a.substr(0, 150) + "...";
                }
                ss << "A: " << a << "\n";
            }
            else
            {
                ss << "A: unresolved\n";
            }

            ss << "Met dependencies:\n";
            for (auto& d : met_dependencies())
            {
                ss << "  " << d->question() << ", " << d->answer() << "\n";
            }

            ss << "Unmet dependencies:\n";

            for (auto& d : unmet_dependencies())
            {
                ss << "  " << d->question() << "\n";
            }

            ss << "Asked by:\n";
            if (m_attached_to)
            {
                for (auto& d : m_attached_to->dependents())
                {
                    ss << "  +" << d->question() << "\n";
                    ss << "Which has debug info:\n";
                    ss << d->debug_recursive();
                }
            }
            else
            {
                for (auto& d : dependents())
                {
                    ss << "  */" << d->question() << "\n";
                    ss << "  Which has debug info:\n";
                    ss << d->debug_recursive();
                }
            }

            return ss.str();
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

        virtual void add_dependency(std::shared_ptr< node_base< Graph > > n)
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
            return !has_unresolved_dependencies() && !resolved() && blocking_coroutine_count() == 0;
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

        void coroutine_callback_suspend_until_ready(coroutine_callback< Graph > handle)
        {
            m_waiting_coroutines.push_back(handle);
        }

        void kickoff_coroutine(coroutine_callback< Graph > cb)
        {
            m_kickoff_coroutines.push_back(cb);
        }

        auto take_waiting_coroutines()
        {
            auto result = std::move(m_waiting_coroutines);

            m_waiting_coroutines.clear();

            return result;
        }

        auto take_kickoff_coroutines()
        {
            auto result = std::move(m_kickoff_coroutines);

            m_kickoff_coroutines.clear();

            return result;
        }

        std::size_t blocking_coroutine_count() const
        {
            return m_kickoff_coroutines.size() + m_busy_coroutines;
        }

        bool is_recursive()
        {
            std::set< node_base< Graph >* > touch_set;
            check_all_transitive_unmet_dependencies(touch_set);
            return touch_set.contains(this);
        }

        std::size_t dep_set_size()
        {
            std::set< node_base< Graph >* > touch_set;
            check_all_transitive_unmet_dependencies(touch_set);
            return touch_set.size();
        }

      private:
        void check_all_transitive_unmet_dependencies(std::set< node_base< Graph >* >& output)
        {
            if (output.contains(this))
            {
                return;
            }

            for (auto d : m_unmet_dependencies)
            {
                output.insert(d);
                d->check_all_transitive_unmet_dependencies(output);
            }

            for (auto d : m_error_dependencies)
            {
                output.insert(d);
                d->check_all_transitive_unmet_dependencies(output);
            }

            for (auto d : m_met_dependencies)
            {
                output.insert(d);
                d->check_all_transitive_unmet_dependencies(output);
            }
        }

        std::set< node_base< Graph >* > m_met_dependencies;
        std::set< node_base< Graph >* > m_unmet_dependencies;
        std::set< node_base< Graph >* > m_error_dependencies;
        std::set< node_base< Graph >* > m_dependents;
        std::vector< coroutine_callback< Graph > > m_kickoff_coroutines;
        std::size_t m_busy_coroutines = 0;
        std::vector< coroutine_callback< Graph > > m_waiting_coroutines;
    };

    template < typename Graph, typename Result >
    class resolver_coroutine;

    template < typename Graph, typename Result >
    class general_coroutine;

    template < typename Graph, typename Result >
    class resolver_base : public virtual node_base< Graph >
    {

      public:
        using value_type = Result;
        using output_type = Result;
        using outptr_type = std::shared_ptr< resolver_base< Graph, Result > >;
        using result_type = result< Result >;

        resolver_base()
        {
        }

        resolver_base(resolver_base< Graph, Result > const&) = delete;

        virtual ~resolver_base() {};

        Result get() const
        {
            assert(m_result.has_result() || m_result.has_error());
            if (m_result.has_error())
            {
                std::rethrow_exception(m_result.get_error());
            }
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
            this->update_dependents();
        }

      public:
        virtual void set_error(std::exception_ptr er) override
        {
            m_result.set_error(er);
            this->update_dependents();
        }

        virtual std::string answer() const override
        {
            if (this->m_attached_to)
            {
                return this->m_attached_to->answer();
            }
            if (has_value())
            {
                return resolver_traits< Result >::stringify(get());
            }
            else if (has_error())
            {
                try
                {
                    std::rethrow_exception(get_error());
                }
                catch (std::exception const& e)
                {
                    return e.what();
                }
                catch (...)
                {
                    return "Unknown error";
                }
            }

            throw std::logic_error("unreachable");
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

            assert(await_ready());
            this->update_dependents();
            //  assert(this->dependents().size() != 0);
            return get();
        }

        // The compiler isn't smart enough to do nested type deduction, so we have to do it manually.
        template < typename Result2 >
        void await_suspend_helper(typename resolver_coroutine< Graph, Result2 >::promise_type& pr)
        {
            pr.cr->add_dependency(this);
            assert(this->dependents().size() != 0);
        }

        template < typename Result2 >
        void await_suspend_helper(typename general_coroutine< Graph, Result2 >::promise_type& pr)
        {
            coroutine_callback< Graph > cb;

            assert(pr.get_coroutine().owning_node != nullptr);

            pr.get_coroutine().waiting_on_node = this;

            cb.callback = [&pr]()
            {
                general_coroutine< Graph, Result2 >& general_coroutine_handle = pr.get_coroutine();
                return general_coroutine_handle.resume();
            };

            assert(pr.get_coroutine().owning_node->m_attached_by == nullptr);

            pr.get_coroutine().owning_node->add_dependency(this);

            assert(this->dependents().size() != 0);
            this->coroutine_callback_suspend_until_ready(cb);
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< T > ch)
        {
            auto& pr = ch.promise();
            using result_type = typename std::remove_reference_t< decltype(pr) >::result_type;
            await_suspend_helper< result_type >(pr);
        }

      private:
        result_type m_result;
    };

    template < typename Graph >
    class resolver_base< Graph, void > : public virtual node_base< Graph >
    {

      public:
        using value_type = void;
        using result_type = result< void >;

        resolver_base()
        {
        }

        resolver_base(resolver_base< Graph, void > const&) = delete;

        virtual ~resolver_base() {};

        void get() const
        {
            return m_result.get();
        }

        std::exception_ptr get_error() const override
        {
            return m_result.get_error();
        }

      protected:
        void set_value()
        {
            m_result.set_value();
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

        void await_resume()
        {
            assert(await_ready());
            get();
        }

        // The compiler isn't smart enough to do nested type deduction, so we have to do it manually.
        template < typename Result2 >
        void await_suspend_helper(typename resolver_coroutine< Graph, Result2 >::promise_type& pr)
        {
            pr.cr->add_dependency(this);
        }

        template < typename Result2 >
        void await_suspend_helper(typename general_coroutine< Graph, Result2 >::promise_type& pr)
        {
            coroutine_callback< Graph > cb;

            pr.get_coroutine().waiting_on_node = this;
            pr.get_coroutine().owning_node = this;

            cb.callback = [&pr]()
            {
                general_coroutine< Graph, Result2 >& general_coroutine_handle = pr.get_coroutine();
                return general_coroutine_handle.resume();
            };

            assert(this->dependents().size() != 0);

            this->coroutine_callback_suspend_until_ready(cb);
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< T > ch)
        {
            auto& pr = ch.promise();
            using result_type = typename std::remove_reference_t< decltype(pr) >::result_type;
            await_suspend_helper< result_type >(pr);
            assert(this->dependents().size() != 0);
        }

      private:
        result_type m_result;
    };

    template < typename Graph, typename Result, typename Input >
    class co_resolver_base : public resolver_base< Graph, Result >
    {
      public:
        using input_type = Input;
        using key_type = Input;
        using co_type = resolver_coroutine< Graph, Result >;

        co_resolver_base(input_type input_val) : input_val(input_val), input(this->input_val)
        {
        }

        virtual ~co_resolver_base()
        {
        }

        void process(Graph* g) override final
        {
            if (!m_coroutine.has_value())
            {
                m_coroutine.emplace(co_process(g, input_val));
                m_coroutine.value().m_attached_to = this;
                this->m_attached_by = &*m_coroutine;
                this->add_dependency(&*m_coroutine);
            }
            else if (this->ready())
            {
                assert(m_coroutine.has_value());
                this->set_value(m_coroutine->get());
            }
        }

        virtual std::string answer() const override
        {
            if (this->has_value())
            {
                std::string result_str;
                std::vector< std::byte > data;
                rpnx::cxx_serialize_iter(this->get(), std::back_inserter(data));

                for (std::byte b : data)
                {
                    result_str += char(b);
                }
                return result_str;
            }
            else if (this->has_error())
            {
                try
                {
                    std::rethrow_exception(this->get_error());
                }
                catch (std::exception const& e)
                {
                    return e.what();
                }
                catch (...)
                {
                    return "Unknown error";
                }
            }
            else
            {
                return "unresolved";
            }
        }

        virtual std::optional< std::string > input_string() const
        {
            return std::nullopt;
        }

        virtual std::string question() const override
        {
            std::string typenam = boost::core::demangle(typeid(*this).name());
            std::vector< std::byte > data;

            auto input_str_opt = input_string();

            std::string input_str;

            if (input_str_opt.has_value())
            {
                input_str = input_str_opt.value();
            }
            else
            {
                rpnx::cxx_serialize_iter(input_val, std::back_inserter(data));

                for (std::byte b : data)
                {
                    input_str += char(b);
                }
            }
            return typenam + "(" + input_str + ")";
        }

      protected:
        input_type input_val;
        input_type const& input;
        virtual resolver_coroutine< Graph, Result > co_process(Graph* g, input_type) = 0;

        std::optional< resolver_coroutine< Graph, Result > > m_coroutine;

        void add_co_dependency(node_base< Graph >* n)
        {
            this->m_coroutine.value().add_dependency(n);
        }

        void add_co_dependency(std::shared_ptr< node_base< Graph > > n)
        {
            this->m_coroutine.value().add_dependency(&*n);
        }
    };

    template < typename Graph, typename Result >
    using output_ptr = std::shared_ptr< resolver_base< Graph, Result > >;

    template < typename Graph >
    class coroutine_base
    {
    };

    // This class implements a general purpose coroutine that can be
    // used with a processor, but which is not itself a resolver.
    // It is expected to be called from within a resolver.
    template < typename Graph, typename Result >
    class general_coroutine
    {
        template < typename Graph2, typename Result2 >
        friend class resolver_coroutine;

        template < typename Graph2, typename Result2 >
        friend class resolver_base;

        template < typename Graph2, typename Result2 >
        friend class general_coroutine;

      public:
        using result_type = Result;

        class promise_type
        {
            friend class general_coroutine< Graph, Result >;

            template < typename Graph2, typename Result2 >
            friend class resolver_coroutine;

          private:
            general_coroutine* cr = nullptr;

          public:
            using result_type = Result;
            using coroutine_type = general_coroutine< Graph, Result >;

            ~promise_type()
            {
                assert(cr->m_result);
                cr->co_promise_ptr = nullptr;
            }

            promise_type(promise_type const&) = delete;

            promise_type()
            {
            }

            template < typename R2 >
            auto& await_transform(general_coroutine< Graph, R2 >& cr)
            {
                return cr;
            }

            template < typename R2 >
            auto& await_transform(general_coroutine< Graph, R2 >&& cr)
            {
                return cr;
            }

            template < typename R2 >
            auto& await_transform(resolver_base< Graph, R2 >& cr)
            {
                assert(this->cr->owning_node != nullptr);
                this->cr->owning_node->add_dependency(&cr);
                return cr;
            }

            template < typename R2 >
            auto& await_transform(resolver_base< Graph, R2 >&& cr)
            {
                this->cr->owning_node->add_dependency(&cr);
                return cr;
            }

            template < typename R2 >
            auto& await_transform(resolver_base< Graph, R2 > const& cr)
            {
                this->cr->owning_node->add_dependency(&cr);
                return cr;
            }

            general_coroutine< Graph, Result >& get_coroutine() const
            {
                assert(cr);
                return *cr;
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
                //  cr->handle_completion();

                return std::suspend_never{};
            }

            void return_value(Result r)
            {
                assert(cr != nullptr);
                assert(!cr->m_result);
                cr->m_result.set_value(r);
            }
        };

      private:
        promise_type* co_promise_ptr = nullptr;
        result< Result > m_result;

        // There are two possible types of things can suspend on us:
        // A resolver_coroutine, or a general_coroutine.
        // Resolver coroutines need to be resumed by the resolver's process function,
        // since the node logic pre-dates usage of C++ coroutines
        // On the other hand, general_coroutines work more like
        // a normal coroutine and we can directly pass callbacks to the processor.

        node_base< Graph >* waiter_node = nullptr;

        node_base< Graph >* owning_node = nullptr;

        std::unique_ptr< coroutine_callback< Graph > > waiter_coroutine;

        std::vector< std::unique_ptr< coroutine_callback< Graph > > > kickoff_coroutines;
        std::optional< node_base< Graph >* > waiting_on_node;
        bool dead = false;

      private:
        general_coroutine(promise_type* arg_ptr_pr) : co_promise_ptr(arg_ptr_pr)
        {
            co_promise_ptr->cr = this;
        }

      public:
        coroutine_exec_result< Graph > resume()
        {
            assert(co_promise_ptr);

            auto co_handle = std::coroutine_handle< promise_type >::from_promise(*co_promise_ptr);
            co_handle.resume();

            if (!m_result)
            {
                assert(co_promise_ptr);
                return get_exec_result();
            }

            assert(co_promise_ptr == nullptr);

            return get_exec_result();
        }

        coroutine_exec_result< Graph > get_exec_result()
        {
            // This should only be called once after we have a result.
            coroutine_exec_result< Graph > result;

            // This unit represents a general subroutine.

            // Either waiter_node is set if the await_suspend was called
            // from a node, or waiter_coroutine is set if the await_suspend
            // was called from a general_coroutine.

            // It is an API contract violation to have multiple await_suspend
            // calls.

            if (m_result)
            {
                assert(waiter_node || waiter_coroutine);
                assert(!(waiter_node && waiter_coroutine));
                assert(kickoff_coroutines.empty());

                if (waiter_node)
                {
                    result.ready_nodes.push_back(waiter_node);
                    waiter_node = nullptr;
                }
                else if (waiter_coroutine)
                {
                    // A coroutine that suspends on us
                    // cannot be waiting on multiple values, and is therefore
                    // always ready
                    result.ready_callbacks.push_back(std::move(waiter_coroutine));
                    waiter_coroutine = nullptr;
                }

                assert(waiter_node == nullptr);
                assert(waiter_coroutine == nullptr);
                return result;
            }
            else
            {
                assert((!kickoff_coroutines.empty()) || (waiting_on_node.has_value()));
                coroutine_exec_result< Graph > result;
                result.ready_callbacks = std::move(kickoff_coroutines);
                kickoff_coroutines.clear();
                if (waiting_on_node.has_value())
                {
                    result.kickoff_nodes.insert(waiting_on_node.value());
                }
                return result;
            }
        }

        general_coroutine()
        {
        }

        bool await_ready() noexcept
        {
            assert(m_result || co_promise_ptr);
            return m_result;
        }

        template < typename Result2 >
        void await_suspend_helper(typename resolver_coroutine< Graph, Result2 >::promise_type& pr)

        {
            auto& waiter_promise = pr;
            assert(waiter_node == nullptr);
            assert(owning_node == nullptr);
            waiter_node = waiter_promise.cr;
            owning_node = waiter_node;
            waiter_node->kickoff_coroutine({[this]()
                                            {
                                                return resume();
                                            }});
        }

        template < typename Result2 >
        void await_suspend_helper(typename general_coroutine< Graph, Result2 >::promise_type& pr)

        {
            auto& waiter_promise = pr;
            assert(waiter_node == nullptr);
            assert(owning_node == nullptr);
            auto& waiter_coroutine = waiter_promise.get_coroutine();
            waiter_coroutine.kickoff_coroutines.push_back(std::make_unique< coroutine_callback< Graph > >(coroutine_callback< Graph >{[this]()
                                                                                                                                      {
                                                                                                                                          return resume();
                                                                                                                                      }}));
            assert(waiter_coroutine.owning_node != nullptr);
            this->owning_node = waiter_coroutine.owning_node;
            this->waiter_coroutine = std::make_unique< coroutine_callback< Graph > >(coroutine_callback< Graph >{[&waiter_coroutine]()
                                                                                                                 {
                                                                                                                     return waiter_coroutine.resume();
                                                                                                                 }});
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< T > handle)
        {
            using promise_type = typename std::remove_reference_t< decltype(handle.promise()) >;
            using coroutine_type = typename promise_type::coroutine_type;
            using result_type = typename promise_type::result_type;
            await_suspend_helper< result_type >(handle.promise());
            assert(this->owning_node != nullptr);
        }

        bool await_resume_already_called = false;

        auto await_resume()
        {
            assert(!await_resume_already_called);
            await_resume_already_called = true;
            return std::move(m_result.get());
        }
    };

    template < typename Graph >
    class general_coroutine< Graph, void >
    {
        template < typename Graph2, typename Result2 >
        friend class resolver_coroutine;

        template < typename Graph2, typename Result2 >
        friend class resolver_base;

        template < typename Graph2, typename Result2 >
        friend class general_coroutine;

      public:
        using result_type = void;

        class promise_type
        {
            friend class general_coroutine< Graph, void >;

            template < typename Graph2, typename Result2 >
            friend class resolver_coroutine;

          private:
            general_coroutine* cr = nullptr;

          public:
            using result_type = void;
            using coroutine_type = general_coroutine< Graph, void >;

            ~promise_type()
            {
                assert(cr->m_result);
                cr->co_promise_ptr = nullptr;
            }

            promise_type(promise_type const&) = delete;

            promise_type()
            {
            }

            general_coroutine< Graph, void >& get_coroutine() const
            {
                assert(cr);
                return *cr;
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
                //  cr->handle_completion();

                return std::suspend_never{};
            }

            void return_void()
            {
                assert(cr != nullptr);
                assert(!cr->m_result);
                cr->m_result.set_value();
            }
        };

      private:
        promise_type* co_promise_ptr = nullptr;
        result< void > m_result;

        // There are two possible types of things can suspend on us:
        // A resolver_coroutine, or a general_coroutine.
        // Resolver coroutines need to be resumed by the resolver's process function,
        // since the node logic pre-dates usage of C++ coroutines
        // On the other hand, general_coroutines work more like
        // a normal coroutine and we can directly pass callbacks to the processor.

        node_base< Graph >* waiter_node = nullptr;

        node_base< Graph >* owning_node = nullptr;

        std::unique_ptr< coroutine_callback< Graph > > waiter_coroutine;

        std::vector< std::unique_ptr< coroutine_callback< Graph > > > kickoff_coroutines;
        std::optional< node_base< Graph >* > waiting_on_node;
        bool dead = false;

      private:
        general_coroutine(promise_type* arg_ptr_pr) : co_promise_ptr(arg_ptr_pr)
        {
            co_promise_ptr->cr = this;
        }

      public:
        coroutine_exec_result< Graph > resume()
        {
            assert(co_promise_ptr);

            auto co_handle = std::coroutine_handle< promise_type >::from_promise(*co_promise_ptr);
            co_handle.resume();

            if (!m_result)
            {
                assert(co_promise_ptr);
                return get_exec_result();
            }

            assert(co_promise_ptr == nullptr);

            return get_exec_result();
        }

        coroutine_exec_result< Graph > get_exec_result()
        {
            // This should only be called once after we have a result.
            coroutine_exec_result< Graph > result;

            // This unit represents a general subroutine.

            // Either waiter_node is set if the await_suspend was called
            // from a node, or waiter_coroutine is set if the await_suspend
            // was called from a general_coroutine.

            // It is an API contract violation to have multiple await_suspend
            // calls.

            if (m_result)
            {
                assert(waiter_node || waiter_coroutine);
                assert(!(waiter_node && waiter_coroutine));
                assert(kickoff_coroutines.empty());

                if (waiter_node)
                {
                    result.ready_nodes.push_back(waiter_node);
                    waiter_node = nullptr;
                }
                else if (waiter_coroutine)
                {
                    // A coroutine that suspends on us
                    // cannot be waiting on multiple values, and is therefore
                    // always ready
                    result.ready_callbacks.push_back(std::move(waiter_coroutine));
                    waiter_coroutine = nullptr;
                }

                assert(waiter_node == nullptr);
                assert(waiter_coroutine == nullptr);
                return result;
            }
            else
            {
                assert((!kickoff_coroutines.empty()) || (waiting_on_node.has_value()));
                coroutine_exec_result< Graph > result;
                result.ready_callbacks = std::move(kickoff_coroutines);
                kickoff_coroutines.clear();
                if (waiting_on_node.has_value())
                {
                    result.kickoff_nodes.insert(waiting_on_node.value());
                }
                return result;
            }
        }

        general_coroutine()
        {
        }

        bool await_ready() noexcept
        {
            assert(m_result || co_promise_ptr);
            return m_result;
        }

        template < typename Result2 >
        void await_suspend_helper(typename resolver_coroutine< Graph, Result2 >::promise_type& pr)

        {
            auto& waiter_promise = pr;
            assert(waiter_node == nullptr);
            waiter_node = waiter_promise.cr;
            owning_node = waiter_node;
            waiter_node->kickoff_coroutine({[this]()
                                            {
                                                return resume();
                                            }});
        }

        template < typename Result2 >
        void await_suspend_helper(typename general_coroutine< Graph, Result2 >::promise_type& pr)

        {
            auto& waiter_promise = pr;
            assert(waiter_node == nullptr);
            auto& waiter_coroutine = waiter_promise.get_coroutine();
            owning_node = waiter_coroutine.owning_node;
            waiter_coroutine.kickoff_coroutines.push_back(std::make_unique< coroutine_callback< Graph > >(coroutine_callback< Graph >{[this]()
                                                                                                                                      {
                                                                                                                                          return resume();
                                                                                                                                      }}));
            this->waiter_coroutine = std::make_unique< coroutine_callback< Graph > >(coroutine_callback< Graph >{[&waiter_coroutine]()
                                                                                                                 {
                                                                                                                     return waiter_coroutine.resume();
                                                                                                                 }});
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< T > handle)
        {
            using promise_type = typename std::remove_reference_t< decltype(handle.promise()) >;
            using coroutine_type = typename promise_type::coroutine_type;
            using result_type = typename promise_type::result_type;
            await_suspend_helper< result_type >(handle.promise());
        }

        auto await_resume()
        {
            return m_result.get();
        }
    };

    template < typename Graph, typename Result >
    class resolver_coroutine : public resolver_base< Graph, Result >
    {
      protected:
      public:
        struct promise_type
        {
            using result_type = Result;

            promise_type()
            {
            }

            promise_type(promise_type const&) = delete;
            using coroutine_type = resolver_coroutine< Graph, Result >;
            resolver_coroutine* cr;

            template < typename R2 >
            auto& await_transform(resolver_base< Graph, R2 >& n)
            {
                this->cr->add_dependency(&n);
                return n;
            }

            template < typename R2 >
            auto& await_transform(general_coroutine< Graph, R2 >& n)
            {
                return n;
            }
            template < typename R2 >
            auto& await_transform(general_coroutine< Graph, R2 >&& n)
            {
                return n;
            }

            template < typename R2 >
            auto& await_transform(general_coroutine< Graph, R2 > const& n)
            {
                return n;
            }

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
        resolver_coroutine(promise_type* pr) : pr(pr)
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

    struct drawer
    {
        std::stringstream ss;
        std::size_t indent = 0;

        template < typename Graph >
        void draw(node_base< Graph >* n)
        {

            if (n->m_attached_by)
            {

                assert(n->error_dependencies().size() + n->met_dependencies().size() <= 1);
                // Note: can be caused by using add_dependency instead of add_co_dependency

                n = n->m_attached_by;

                assert(n->m_attached_by == nullptr);
            }

            for (std::size_t i = 0; i < indent; i++)
            {
                ss << " ";
            }

            ss << "ASK " << n->question() << " WITH ";
            if (n->has_value())
            {
                std::string answer = n->answer();
                if (answer.size() > 100)
                {
                    answer = answer.substr(0, 100) + "...";
                }
                ss << "ANSWER " << answer << "\n";
            }
            else if (n->has_error())
            {
                ss << "ERROR " << n->answer() << "\n";
            }
            else if (n->has_error())
            {
                ss << "FAILED (dependency failed)\n";
            }

            ss << "\n";

            indent += 4;

            // ss << n->error_dependencies().size() << " error deps\n";
            // ss << n->met_dependencies().size() << " met deps\n";

            for (auto& d : n->met_dependencies())
            {
                draw(d);
            }
            for (auto& d : n->error_dependencies())
            {
                draw(d);
            }

            indent -= 4;
        }
    };

    template < typename Graph >
    class single_thread_graph_solver
    {

      public:
        inline void solve(Graph* graph, std::shared_ptr< node_base< Graph > > node)
        {
            std::exception_ptr eptr;
            try
            {
                solve(graph, node.get());
            }
            catch (...)
            {
                eptr = std::current_exception();
            }

            drawer d;
            d.draw(node.get());
            std::string result = d.ss.str();
            std::cout << d.ss.str() << std::endl;

            if (eptr)
            {
                std::rethrow_exception(eptr);
            }
        }

        void solve(Graph* graph, node_base< Graph >* node)
        {
            std::set< node_base< Graph >* > m_all_nodes;

            std::set< node_base< Graph >* > nodes_to_process;

            std::vector< coroutine_callback< Graph > > coroutines_to_process;
            nodes_to_process.insert(node);
            m_all_nodes.insert(node);

            while (nodes_to_process.size() > 0 || coroutines_to_process.size() > 0)
            {
                if (coroutines_to_process.size() != 0)
                {
                    coroutine_callback< Graph > co_callback_exec = std::move(coroutines_to_process.back());
                    coroutines_to_process.pop_back();

                    auto func = co_callback_exec.callback;
                    assert(func);
                    auto results = func();
                    // auto results = co_callback_exec.callback();

                    assert(results.ready_callbacks.size() != 0 || results.ready_nodes.size() != 0 || results.kickoff_nodes.size() != 0);

                    assert(results.ready_nodes.size() <= 1);
                    for (node_base< Graph >* node_new : results.ready_nodes)
                    {
                        assert(node_new->m_busy_coroutines > 0);
                        node_new->m_busy_coroutines--;
                        if (node_new->ready())
                        {
                            nodes_to_process.insert(node_new);
                        }
                    }

                    for (node_base< Graph >* node_new : results.kickoff_nodes)
                    {
                        if (node_new->ready())
                        {
                            nodes_to_process.insert(node_new);
                        }
                    }

                    for (std::unique_ptr< coroutine_callback< Graph > >& co_callback_new_ptr : results.ready_callbacks)
                    {
                        coroutines_to_process.push_back(std::move(*co_callback_new_ptr));
                    }

                    continue;
                }

                node_base< Graph >* n = *nodes_to_process.begin();
                m_all_nodes.insert(n);
                nodes_to_process.erase(n);
                if (n->resolved() || !n->ready())
                {
                    continue;
                }

                try
                {
                    n->out() = std::stringstream();
                    n->process(graph);
                }
                catch (...)
                {
                    assert(!n->has_value());
                    n->set_error(std::current_exception());
                }

                if (!(n->resolved() || n->has_unresolved_dependencies() || n->blocking_coroutine_count() != 0))
                {
                    QUXLANG_DEBUG({ std::cout << "failed resolution:" << n->question() << std::endl; });
                }
                assert(n->resolved() || n->has_unresolved_dependencies() || n->blocking_coroutine_count() != 0);

                if (n->resolved() && !n->m_attached_to)
                {

                    if (n->has_value())
                    {
                        QUXLANG_DEBUG({ std::cout << "Q: " << n->question() << " A: " << n->answer() << std::endl; });
                    }
                    else
                    {
                        QUXLANG_DEBUG({ std::cout << "Q: " << n->question() << " A: error" << std::endl; });
                    }
                }

                for (auto& dep : n->met_dependencies())
                {
                    m_all_nodes.insert(dep);
                }

                for (auto& dep : n->error_dependencies())
                {
                    m_all_nodes.insert(dep);
                }

                for (auto& dep : n->unmet_dependencies())
                {
                    m_all_nodes.insert(dep);
                }

                auto kickoff_coroutines = n->take_kickoff_coroutines();
                coroutines_to_process.insert(coroutines_to_process.end(), kickoff_coroutines.begin(), kickoff_coroutines.end());
                n->m_busy_coroutines += kickoff_coroutines.size();

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
                    assert(requirements.size() != 0 || n->blocking_coroutine_count() != 0);
                }
            }

            for (auto const& n : m_all_nodes)
            {
                if (!n->resolved())
                {
                    QUXLANG_DEBUG({ std::cout << "missing:" << n->question() << std::endl; });

                    for (auto const& n : m_all_nodes)
                    {
                        if (!n->resolved())
                        {
                            QUXLANG_DEBUG({ std::cout << "Not resolved: " << n->question() << std::endl; });
                            QUXLANG_DEBUG({ std::cout << "Touch set size: " << n->dep_set_size() << std::endl; });
                        }
                    }
                    for (auto const& n : m_all_nodes)
                    {
                        if (n->is_recursive())
                        {
                            QUXLANG_DEBUG({ std::cout << "RECURSIVE: " << n->question() << std::endl; });
                        }
                    }
                    throw std::runtime_error("Could not resolve node, probably recursive dependency (2)");
                }
                // QUXLANG_DEBUG({std::cout << n->debug() << std::endl;});
            }

            if (!node->resolved())
            {
                QUXLANG_DEBUG({ std::cout << "missing:" << node->question() << std::endl; });
                throw std::runtime_error("Could not resolve node, probably recursive dependency (1)");
            }
        }
    };

    template < typename T >
    concept awaitable = requires(T&& t, std::coroutine_handle<> h) {
        { t.await_ready() } -> std::convertible_to< bool >;
        t.await_resume();
    };

    template < typename Graph, awaitable T >
    auto co_call(T t) -> general_coroutine< Graph, typename T::result_type >
    {
        co_return co_await t;
    }

    template < typename Graph, typename T >
        requires(!awaitable< T >)
    auto co_call(T t) -> general_coroutine< Graph, T >
    {
        co_return t;
    }

} // namespace rpnx

#endif