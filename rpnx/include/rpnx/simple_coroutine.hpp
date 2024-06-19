//
// Created by Ryan Nicholl on 6/12/24.
//

#ifndef RPNX_QUXLANG_SIMPLE_COROUTINE_HEADER
#define RPNX_QUXLANG_SIMPLE_COROUTINE_HEADER

#include <cassert>
#include <coroutine>
#include <memory>
#include <rpnx/result.hpp>

namespace rpnx
{
    class simple_coroutine_state_base;

    template < typename T >
    class simple_coroutine;

    class simple_coroutine_runner
    {
        std::vector< std::shared_ptr< simple_coroutine_state_base > > m_run_queue;

      public:
        void run(std::shared_ptr< simple_coroutine_state_base > state);
    };
    class simple_coroutine_state_base : public std::enable_shared_from_this< simple_coroutine_state_base >
    {
        friend class simple_coroutine_runner;

      protected:
        std::vector< simple_coroutine_state_base* > m_waiting_on_me;

      public:
        virtual ~simple_coroutine_state_base() = default;
        virtual bool has_value() const = 0;
        virtual bool has_error() const = 0;
        virtual bool has_result() const = 0;
        virtual void test() const = 0;

        virtual bool await_ready() const = 0;

        template < typename T >
        void await_suspend(std::coroutine_handle< typename simple_coroutine< T >::promise_type > handle)
        {
            assert(!await_ready());
            auto state_ptr = handle.promise().state->shared_from_this();
            // simple_coroutine< T > cr(state_ptr);
            m_waiting_on_me.push_back(state_ptr);
        }

        virtual std::vector< simple_coroutine_state_base* > resume() = 0;
    };

    template < typename T >
    class simple_coroutine;
    template <>
    class simple_coroutine< void >;

    template <>
    class simple_coroutine< void >
    {
        friend class simple_coroutine_runner;

      public:
        class promise_type;

      private:
        struct state : simple_coroutine_state_base
        {
            friend class simple_coroutine< void >;
            rpnx::result< void > m_result;
            promise_type* m_pr{};

            state(promise_type* pr)
                : m_pr(pr)
            {
            }

            bool has_value() const override
            {
                return m_result.has_value();
            }
            bool has_error() const override
            {
                return m_result.has_error();
            }
            bool has_result() const override
            {
                return m_result.has_result();
            }

            void test() const override
            {
                m_result.test();
            }

            bool await_ready() const override
            {
                return m_result.has_result();
            }

            virtual std::vector< simple_coroutine_state_base* > resume() override
            {
                std::coroutine_handle< promise_type > handle = std::coroutine_handle< promise_type >::from_promise(*m_pr);
                handle.resume();

                if (m_result.has_result())
                {
                    auto result = std::move(m_waiting_on_me);
                    m_waiting_on_me.clear();
                    return result;
                }
                else
                {
                    return {};
                }
            }

            void await_resume()
            {
                return m_result.get();
            }
        };

        std::shared_ptr< state > m_state;

      private:
        simple_coroutine(std::shared_ptr< state > state)
        {
            m_state = std::move(state);
        }

      public:
        class promise_type
        {
            friend class simple_coroutine;

          private:
            simple_coroutine< void >::state* m_coro_state{};

          public:
            simple_coroutine get_return_object()
            {
                auto result_state = std::make_shared< simple_coroutine< void >::state >(this);
                return simple_coroutine(std::move(result_state));
            }

            auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                return std::suspend_always{};
            }

            void return_void()
            {
                m_coro_state->m_result.set_value();
            }

            void unhandled_exception()
            {
                m_coro_state->m_result.set_error(std::current_exception());
            }
        };

        void await_sync()
        {
            simple_coroutine_runner runner;
            runner.run(this->m_state);
            return this->m_state->m_result.get();
        }

        bool await_ready() const noexcept
        {
            return this->m_state->m_result.has_value();
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< typename simple_coroutine< T >::promise_type > handle)
        {
            auto& promise = handle.promise();
            auto state = handle.promise().m_coro_state;
            this->m_state->m_waiting_on_me.push_back(state);
        }

        void await_resume()
        {
            return this->m_state->m_result.get();
        }
    };

    template < typename T >
    class simple_coroutine
    {
        friend class simple_coroutine_runner;

      public:
        class promise_type;

      private:
        struct state : simple_coroutine_state_base
        {
            friend class simple_coroutine< T >;
            rpnx::result< T > m_result;
            promise_type* m_pr{};

            state(promise_type* pr)
                : m_pr(pr)
            {
            }
            bool has_value() const override
            {
                return m_result.has_value();
            }
            bool has_error() const override
            {
                return m_result.has_error();
            }
            bool has_result() const override
            {
                return m_result.has_result();
            }

            void test() const override
            {
                m_result.test();
            }

            bool await_ready() const override
            {
                return m_result.has_result();
            }

            virtual std::vector< simple_coroutine_state_base* > resume() override
            {
                std::coroutine_handle< promise_type > handle = std::coroutine_handle< promise_type >::from_promise(*m_pr);
                handle.resume();

                if (m_result.has_result())
                {
                    auto result = std::move(m_waiting_on_me);
                    m_waiting_on_me.clear();
                    return result;
                }
                else
                {
                    return {};
                }
            }

            T await_resume()
            {
                return m_result.get();
            }
        };

        std::shared_ptr< state > m_state;

      private:
        simple_coroutine(std::shared_ptr< state > state)
        {
            m_state = std::move(state);
        }

      public:
        class promise_type
        {
            friend class simple_coroutine;

          private:
            simple_coroutine< T >::state* m_coro_state{};

          public:
            simple_coroutine get_return_object()
            {
                auto result_state = std::make_shared< simple_coroutine< T >::state >(this);
                return simple_coroutine(std::move(result_state));
            }

            auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                return std::suspend_always{};
            }

            void return_value(T value)
            {
                m_coro_state->m_result = value;
            }

            void unhandled_exception()
            {
                m_coro_state->m_result.set_error(std::current_exception());
            }
        };

        T await_sync()
        {
            simple_coroutine_runner runner;
            runner.run(this->m_state);
            return this->m_state->m_result.value();
        }

        bool await_ready() const noexcept
        {
            return this->m_state->m_result.has_value();
        }

        template < typename U >
        void await_suspend(std::coroutine_handle< typename simple_coroutine< U >::promise_type > handle)
        {
            auto state = handle.promise().m_coro_state;
            this->m_state->m_waiting_on_me.push_back(state);
        }

        T await_resume()
        {
            return this->m_state->m_result.get();
        }
    };

    template < typename T >
    class awaitable_result
    {
        rpnx::result< T > result;

      public:
        awaitable_result(rpnx::result< T > value)
            : result(value)
        {
        }

        awaitable_result(T value)
            : result(value)
        {
        }

        awaitable_result(std::exception_ptr exception)
        {
            result.set_error(exception);
        }
        T await_resume()
        {
            return result.get();
        }
        bool await_ready()
        {
            return true;
        }
        void await_suspend(std::coroutine_handle<> handle)
        {
            handle.resume();
        }
    };

    inline void simple_coroutine_runner::run(std::shared_ptr< simple_coroutine_state_base > state)
    {
        std::vector< simple_coroutine_state_base* > run_queue;
        run_queue.push_back(state.get());
        while (!run_queue.empty())
        {
            auto current = run_queue.back();
            run_queue.pop_back();
            auto new_states = current->resume();
            run_queue.insert(run_queue.end(), new_states.begin(), new_states.end());
        }
    }

} // namespace rpnx

#endif // RPNX_QUXLANG_SIMPLE_COROUTINE_HEADER
