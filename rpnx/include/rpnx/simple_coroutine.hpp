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
        template < typename U >
        friend class simple_coroutine;

        friend class simple_coroutine_runner;

      protected:
        std::set< simple_coroutine_state_base* > m_waiting_on_me;
        simple_coroutine_state_base* m_waiting_on{};

      public:
        virtual ~simple_coroutine_state_base() = default;
        virtual bool has_value() const = 0;
        virtual bool has_error() const = 0;
        virtual bool has_result() const = 0;
        virtual void test() const = 0;

        virtual bool await_ready() const = 0;

        bool check()
        {
            assert(!await_ready() || m_waiting_on == nullptr);
            if (m_waiting_on != nullptr)
            {
                assert(m_waiting_on->m_waiting_on_me.find(this) != m_waiting_on->m_waiting_on_me.end());
            }
            for (auto state : m_waiting_on_me)
            {
                assert(state->m_waiting_on == this);
            }
            return true;
        }

        template < typename T >
        void await_suspend(std::coroutine_handle< typename simple_coroutine< T >::promise_type > handle)
        {
            assert(check());
            assert(!await_ready());
            auto state_ptr = handle.promise().state->shared_from_this();
            // simple_coroutine< T > cr(state_ptr);
            m_waiting_on_me.insert(state_ptr);
            assert(state_ptr->m_waiting_on == nullptr);
            state_ptr->m_waiting_on = this;

            assert(check());
            assert(state_ptr->check());
        }

        virtual std::set< simple_coroutine_state_base* > resume() = 0;
        std::set< simple_coroutine_state_base* > waiting_on() const
        {
            if (m_waiting_on != nullptr)
            {
                return {m_waiting_on};
            }
            else
            {
                return {};
            }
        }

        bool resumable() const
        {
            return m_waiting_on == nullptr && !await_ready();
        }

        bool ready() const
        {
            return await_ready() || resumable();
        }
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
            template < typename U >
            friend class simple_coroutine;
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

            virtual std::set< simple_coroutine_state_base* > resume() override
            {
                std::coroutine_handle< promise_type > handle = std::coroutine_handle< promise_type >::from_promise(*m_pr);
                handle.resume();

                if (m_result.has_result())
                {
                    auto result = std::move(m_waiting_on_me);
                    for (auto state : result)
                    {
                        state->m_waiting_on = nullptr;
                    }
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
            template < typename U >
            friend class simple_coroutine;

          private:
            simple_coroutine< void >::state* m_coro_state{};

          public:
            using future_type = simple_coroutine< void >;
            simple_coroutine get_return_object()
            {
                auto result_state = std::make_shared< simple_coroutine< void >::state >(this);
                m_coro_state = result_state.get();
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
            return this->m_state->m_result.has_result();
        }

        template < typename U >
        void await_suspend_helper(typename U::promise_type& pr)
        {
            auto state = pr.m_coro_state;
            this->m_state->m_waiting_on_me.insert(state);
            state->m_waiting_on = this->m_state.get();
            state->check();
            this->m_state->check();
        }

        template < typename U >
        void await_suspend(std::coroutine_handle< U > ch)
        {
            U& pr = ch.promise();
            using future_type = typename U::future_type;
            await_suspend_helper< future_type >(pr);
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
            template < typename U >
            friend class simple_coroutine;

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

            virtual std::set< simple_coroutine_state_base* > resume() override
            {
                std::coroutine_handle< promise_type > handle = std::coroutine_handle< promise_type >::from_promise(*m_pr);
                handle.resume();

                if (m_result.has_result())
                {
                    auto result = std::move(m_waiting_on_me);
                    m_waiting_on_me.clear();
                    for (auto state : result)
                    {
                        state->m_waiting_on = nullptr;
                    }
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
            template < typename U >
            friend class simple_coroutine;

          private:
            simple_coroutine< T >::state* m_coro_state{};

          public:
            using future_type = simple_coroutine< T >;
            simple_coroutine get_return_object()
            {
                auto result_state = std::make_shared< simple_coroutine< T >::state >(this);
                this->m_coro_state = result_state.get();
                return simple_coroutine(std::move(result_state));
            }

            auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            auto final_suspend() noexcept
            {
                assert(this->m_coro_state->m_waiting_on == nullptr);
                return std::suspend_always{};
            }

            void return_value(T value)
            {
                assert(this->m_coro_state != nullptr);
                assert(!this->m_coro_state->m_result.has_result());
                assert(this->m_coro_state->m_waiting_on == nullptr);
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
            assert(this->m_state->waiting_on().empty());
            assert(await_ready());
            return this->m_state->m_result.get();
        }

        bool await_ready() const noexcept
        {
            return this->m_state->m_result.has_result();
        }

        template < typename U >
        void await_suspend_helper(typename U::promise_type& pr)
        {
            assert(this->m_state != nullptr);
            auto state = pr.m_coro_state;
            assert(state != nullptr);
            this->m_state->m_waiting_on_me.insert(state);
            state->m_waiting_on = this->m_state.get();
        }

        template < typename U >
        void await_suspend(std::coroutine_handle< U > ch)
        {
            U& pr = ch.promise();
            using future_type = typename U::future_type;
            await_suspend_helper< future_type >(pr);
        }

        T await_resume()
        {
            assert(await_ready());
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
            assert(await_ready());
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

    template <>
    class awaitable_result< void >
    {
        rpnx::result< void > result;

      public:
        awaitable_result()
        {
            result.set_value();
        }

        awaitable_result(std::exception_ptr exception)
        {
            result.set_error(exception);
        }
        void await_resume()
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
        int event = 0;
        std::set< simple_coroutine_state_base* > run_queue;
        std::cout << "initial state " << uintptr_t(state.get()) << std::endl;
        run_queue.insert(state.get());
        while (!run_queue.empty())
        {
            event++;
            //std::cout << "event " << event << std::endl;
            auto current = *run_queue.begin();
            run_queue.erase(run_queue.begin());
            //std::cout << "processing " << uintptr_t(current) << std::endl;
            assert(current->check());
            assert(!current->await_ready());

            if (current->resumable())
            {
               // std::cout << "resuming " << uintptr_t(current) << std::endl;
                auto new_states = current->resume();
                for (auto new_state : new_states)
                {
                    //std::cout << "after resume, " << uintptr_t(current) << " wakes " << uintptr_t(new_state) << std::endl;
                    assert(new_state->m_waiting_on == nullptr);
                    run_queue.insert(new_state);
                }
                current->check();
                if (current->resumable())
                {
                   // std::cout << "resumable " << uintptr_t(current) << std::endl;
                    run_queue.insert(current);
                }
                current->check();
                assert(!new_states.empty() || current->resumable() || current->await_ready() || current->m_waiting_on != nullptr);
            }

            if (current->m_waiting_on != nullptr)
            {
               // std::cout << "waiting, " << uintptr_t(current) << " waiting on " << uintptr_t(current->m_waiting_on) << std::endl;
                run_queue.insert(current->m_waiting_on);
                current->m_waiting_on->check();
                current->check();
            }
            assert(current->await_ready() || current->m_waiting_on != nullptr);
            current->check();
        }

        assert(state->check());
        assert(state->await_ready());
        assert(state->m_waiting_on == nullptr);
    }

} // namespace rpnx

#endif // RPNX_QUXLANG_SIMPLE_COROUTINE_HEADER
