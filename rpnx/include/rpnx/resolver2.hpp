// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef RESOLVER2_HPP
#define RESOLVER2_HPP

#include <rpnx/serializer.hpp>
#include <shared_mutex>
#include <set>
#include <vector>
#include <coroutine>

namespace rpnx
{
    class graph_base;

    template < typename T >
    class graph_node_output_state;

    template < typename T >
    class graph_fragment;

    class graph_fragment_any;
    class graph_fragment_state_base;
} // namespace rpnx

namespace rpnx
{
    class graph_base
    {
    };

    class graph_fragment_generator_base
    {
    public:
        virtual ~graph_fragment_generator_base() = default;
        virtual graph_fragment_any generate_node(std::vector<std::byte> input) = 0;
    };

    class graph_fragment_any
    {
        std::shared_ptr< graph_fragment_state_base > m_state;

      public:
        template < typename T >
        graph_fragment_any(graph_fragment< T > node)
        {
            m_state = node.m_state;
        }

        graph_fragment_any()
        {
        }
        graph_fragment_any(graph_fragment_any const&) = default;
        graph_fragment_any(graph_fragment_any&&) = default;

        template < typename T >
        graph_fragment_any& operator=(graph_fragment< T > node)
        {
            m_state = node.m_state;
            return *this;
        }

        graph_fragment_any& operator=(graph_fragment_any const&) = default;
    };

    template < typename T >
    class graph_fragment
    {

        std::shared_ptr< graph_node_output_state< T > > m_state;

      public:
        struct promise_type
        {
            graph_node_output_state< T >* m_state = nullptr;

            auto get_return_object()
            {
                auto state = std::make_shared< graph_node_output_state< T > >{this};
                return graph_fragment< T >{state};
            }

            auto initial_suspend()
            {
                return std::suspend_always{};
            }

            auto return_value(T t)
            {
                m_state->set_result(std::move(t));
                return std::suspend_never{};
            }
        };

        graph_fragment(graph_base* root, T t)
        {
            m_state = std::make_shared< graph_node_output_state< T > >(nullptr);
            m_state->set_root(root);
            m_state->set_result(std::move(t));
        }

        explicit graph_fragment(std::shared_ptr< graph_node_output_state< T > > p)
            : m_state(p)
        {
        }

        ~graph_fragment()
        {
        }

        template < typename T2 >
        auto await_suspend(T2 co)
        {
            return m_state->await_suspend(co);
        }
    };
    class graph_fragment_state_base;

    template < typename T >
    class graph_node_subcoroutine
    {
        struct graph_node_subcoroutine_state;
        struct promise_type
        {
            graph_node_subcoroutine_state* m_state = nullptr;
        };
        struct graph_node_subcoroutine_state
        {
            promise_type* m_prom = nullptr;
            graph_fragment_state_base* m_base = nullptr;
            std::optional< std::coroutine_handle< void > > waiter;
            rpnx::result< T > m_result;

            graph_node_subcoroutine_state(promise_type* p)
            {
                m_prom = p;
                p->m_state = this;
            }

            void set_result(T t)
            {
                assert(!m_result);
                try
                {
                    m_result.set_value(std::move(t));

                    auto co = waiter.value();

                    assert(m_base != nullptr);

                    std::unique_lock lck(m_base->m_mutex);
                    m_base->m_exec_ready.push_back(co);
                }
                catch (...)
                {
                    m_result = {};
                    throw;
                }

                waiter = std::nullopt;
            }
        };

        std::weak_ptr< graph_fragment_state_base > m_state;
    };

    class graph_fragment_state_base
    {
      protected:
        std::mutex m_mutex;
        graph_base* m_root = nullptr;
        std::vector< graph_fragment_state_base* > m_sequence;
        std::set< graph_fragment_state_base* > m_dependencies;
        std::set< graph_fragment_state_base* > m_dependents;

        // When a coroutine frame is ready to execute, it is placed in m_exec_ready to be dispatched.
        std::vector< std::coroutine_handle< void > > m_exec_ready;

        // When another coroutine waits on this node, it is placed in m_exec_wait.
        std::vector< std::coroutine_handle< void > > m_exec_wait;

      public:
        virutal ~graph_fragment_state_base()
        {
        }

        graph_fragment_state_base(graph_fragment_state_base const&) = delete;
        graph_fragment_state_base(graph_fragment_state_base&&) = delete;

        virtual std::string question_kind() const = 0;
        virtual std::string question_input_type() const = 0;
        virtual std::string question_output_type() const = 0;

        virtual std::string question_input_string() = 0;
        virtual std::vector< std::byte > question_input_binary() = 0;
        virtual std::string output_string() = 0;
        virtual std::vector< std::byte > output_binary() = 0;



        virtual bool completed() = 0;

      protected:
        void set_root(graph_base* root)
        {
            m_root = root;
        }


        std::suspend_always await_suspend_state(std::coroutine_handle< void > co, graph_fragment_state_base* dep)
        {
            assert(m_root != nullptr);

            std::unique_lock lck(dep->m_mutex);

            m_dependents.insert(dep);

            if (!completed())
            {
                m_exec_wait.push_back(co);
            }
            else
            {
                m_exec_ready.push_back(co);
            }
        }
    };

    template < typename T >
    class graph_node_output_state : public graph_fragment_state_base
    {
        rpnx::result< T > m_result;
        promise_type* m_promise = nullptr;

      public:
        graph_node_output_state(graph_node_output_state< T > const&) = delete;
        graph_node_output_state(graph_node_output_state< T >&&) = delete;
        graph_node_output_state(promise_type* p)
            : m_promise(p)
        {
            if (p != nullptr)
            {
                p->m_state = this;
            }
        }

        virtual question_output_type() const
        {
            return rpnx::demangle(typeid(T).name());
        }

        virtual std::string output_string() override
        {
            std::vector< std::byte > json_bytes;
            rpnx::json_serialize_iter(m_result.get(), std::back_inserter(json_bytes));
            return std::string(json_bytes.begin(), json_bytes.end());
        }

        virtual std::vector< std::byte > output_binary() override
        {
            std::vector< std::byte > bytes;
            rpnx::serialize_iter(m_result.get(), std::back_inserter(bytes));
            return bytes;
        }

        bool await_ready()
        {
            return m_result.has_value() || m_result.has_error();
        }

        template < typename T2 >
        auto await_suspend(std::coroutine_handle< graph_fragment< T2 >::promise_type > co)
        {
            auto state = co.promise().m_state;

            return await_suspend_state(co, state);
        }

        template < typename T2 >
        auto await_suspend(std::coroutine_handle< graph_node_subcoroutine< T2 >::promise_type > co)
        {
            auto state = co.promise().m_state;

            return await_suspend_state(co, state);
        }

      protected:
        void set_result(T t)
        {

            std::lock_guard< std::mutex > lock(m_mutex);

            assert(m_root);
            m_result.set_value(std::move(t));
            while (!m_exec_wait.empty())
            {
                m_exec_ready.push_back(std::move(m_exec_wait.back()));
                m_exec_wait.pop_back();
            }
        }


    };

    // A pure question is a question that depends only on the input and
    // any sub-questions it may ask.
    // It can be cached between runs if the executable version is the same.
    // Note the dependencies don't have to be pure, only the question itself.
    // Example: "class_placement_info_question"
    struct pure_question_tag
    {
    };

    // An impure question depends upon the state of the system,
    // other than the input and sub-questions.
    // Cannot be cached between runs.
    // Example: "input_file_list_question"
    struct impure_question_tag
    {
    };

    /**
     * A specialization of question-traits should implement:
     *  input_type
     *  output_type
     *  name (string)
     *  */
    template < typename T >
    struct question_traits;

    /**
     * A specialization of question_impl_traits should implement:
     *
     * question_tag (pure_question_tag or impure_question_tag)
     *
     * template <typename Co>
     * Co co_process(G & graph, question_traits<Q>::input_type input);
     *
     * */
    template < typename G, typename Q >
    struct question_impl_traits;

    class graph_resolver : public graph_base
    {
      public:
        using question_func = std::function< graph_fragment_any(std::vector< std::byte >) >;

      private:
        std::map< std::string, question_func > m_generator;

        struct resolver_cache
        {
            std::shared_mutex m;
            std::map<std::vector<std::byte>, graph_fragment_any> m_cache;
        };

        std::map< std::string, resolver_cache > m_caches;

      protected:
        void register_resolver_impl(std::string name, question_func func)
        {
            m_generator[name] = func;
        }

      public:
        virtual ~graph_resolver()
        {
        }

        graph_fragment_any ask(std::string resolver, std::vector<std::byte> input_arg)
        {

        }
    };

    template <typename Q>
    auto ask(graph_resolver & q, typename question_traits<Q>::input_type input)
    {
        std::vector<std::byte> input_bytes;
        rpnx::serialize_iter(input, std::back_inserter(input_bytes));

        node_name name;
        name.resolver_name = question_traits<Q>::name;



    }

    template < typename Q >
    class static_answerer : public virtual graph_resolver
    {
        std::map< question_traits< Q >::input_type, question_traits< Q >::output_type > m_answers;

        friend class question_impl_traits< static_answerer< Q >, Q >;

      public:
        static_answerer(std::map< question_traits< Q >::input_type, question_traits< Q >::output_type > answers)
        {
            m_answers = std::move(answers);
            register_self();
        }

        template < typename It >
        static_answerer(It begin, It end)
        {

            for (auto it = begin; it != end; ++it)
            {
                m_answers[it->first] = it->second;
            }
            register_self();
        }

      private:
        void register_self()
        {
            register_resolver_impl(question_traits< Q >::name,
                                   [this](std::vector< std::byte > input)
                                   {
                                       return ask(input);
                                   });
        }

        graph_fragment_any ask(std::vector< std::byte > input)
        {
            question_traits< Q >::input_type in;
            rpnx::deserialize_iter(in, input.begin(), input.end());
                auto it = m_answers.find(in);
            if (it == m_answers.end())
            {
                throw std::runtime_error("No answer for input");
            }
            return graph_fragment<question_traits<Q>::output_type>(this, it->second);
        }
    };

    template < typename Q >
    struct question_impl_traits< static_answerer< Q >, Q >
    {
        using question_tag = impure_question_tag;

        template < typename Co >
        Co co_process(static_answerer< Q >& graph, question_traits< Q >::input_type input)
        {
            co_return graph.m_answers[input];
        }
    };

    class question_coroutine
    {
      public:
        class promise_type
        {
        };
    };
} // namespace rpnx
#endif // RESOLVER2_HPP