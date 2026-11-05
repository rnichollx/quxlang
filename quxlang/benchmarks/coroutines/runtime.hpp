// Copyright 2026 Ryan P. Nicholl
#ifndef QUXLANG_BENCHMARK_COROUTINE_RUNTIME_HPP
#define QUXLANG_BENCHMARK_COROUTINE_RUNTIME_HPP

#include <coroutine>
#include <cstdint>
#include <exception>
#include <utility>

namespace coroutine_benchmark
{
    /** Owns one lazily started coroutine and drives it on the calling thread. */
    struct task
    {
        /** Stores a completion value or an exception in the coroutine frame. */
        struct promise_type
        {
            std::uint64_t value = 0;
            std::exception_ptr error;
            /** Connects the returned owner with the compiler-allocated frame. */
            task get_return_object() { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
            /** Defers execution until the owner drives the invocation. */
            std::suspend_always initial_suspend() noexcept { return {}; }
            /** Preserves the completed frame until the owner destroys it. */
            std::suspend_always final_suspend() noexcept { return {}; }
            /** Stores the coroutine's result. */
            void return_value(std::uint64_t result) noexcept { value = result; }
            /** Preserves an exception for the caller. */
            void unhandled_exception() noexcept { error = std::current_exception(); }
        };
        std::coroutine_handle<promise_type> frame;
        /** Acquires a newly created coroutine frame. */
        explicit task(std::coroutine_handle<promise_type> value) : frame(value) {}
        task(task const&) = delete;
        /** Transfers ownership of the frame. */
        task(task&& other) noexcept : frame(std::exchange(other.frame, {})) {}
        /** Releases the frame and its captured parameters. */
        ~task() { if (frame) frame.destroy(); }
        /** Drives each explicitly requested suspension to completion. */
        void run() { while (!frame.done()) frame.resume(); }
        /** Retrieves the completed result. */
        std::uint64_t result()
        {
            if (frame.promise().error) std::rethrow_exception(frame.promise().error);
            return frame.promise().value;
        }
        /** Drives a root invocation and retrieves its result. */
        std::uint64_t get() { run(); return result(); }
        /** Requires the child invocation to be driven before retrieving its result. */
        bool await_ready() const noexcept { return false; }
        /** Completes the child inline, allowing the parent to continue in its activation. */
        bool await_suspend(std::coroutine_handle<>) { run(); return false; }
        /** Retrieves the child result after its driver completes. */
        std::uint64_t await_resume() { return result(); }
    };
}
#endif
