// Copyright (c) 2026 Ryan P. Nicholl <rnicholl@protonmail.com>

#ifndef QUXLANG_PROFILING_HPP
#define QUXLANG_PROFILING_HPP

#define QUXLANG_ENABLE_PROFILING true
#include <chrono>
#include <functional>
#include <iostream>
#include <source_location>
#include <string>
#include <utility>

namespace quxlang
{
    static const auto profile_threshold = std::chrono::microseconds(1000);

    void run_under_profiling_void(std::string const& context, auto&& func, std::source_location src = std::source_location::current())
    {
        auto start_time = std::chrono::steady_clock::now();
        std::forward< decltype(func) >(func)();
        auto end_time = std::chrono::steady_clock::now();

        auto duration = end_time - start_time;

        if (duration > profile_threshold)
        {
            std::cerr << "[quxlang:" << src.file_name() << ":" << src.line() << "] ";
            if (!context.empty())
            {
                std::cerr << "'" << context << "' ";
            }
            std::cerr << double(duration.count()) / 1000 / 1000 << "ms\n";
        }
    }

    void run_under_profiling_void(auto&& func, std::source_location src = std::source_location::current())
    {
        run_under_profiling_void(std::string(), std::forward< decltype(func) >(func), src);
    }

    void run_under_profiling_void(std::function< std::string() > const& context, auto&& func, std::source_location src = std::source_location::current())
    {
        auto start_time = std::chrono::steady_clock::now();
        std::forward< decltype(func) >(func)();
        auto end_time = std::chrono::steady_clock::now();

        auto duration = end_time - start_time;

        if (duration > profile_threshold)
        {
            std::cerr << "[quxlang:" << src.file_name() << ":" << src.line() << "] ";
            auto const context_str = context();
            if (!context_str.empty())
            {
                std::cerr << "'" << context_str << "' ";
            }
            std::cerr << double(duration.count()) / 1000 / 1000 << "ms\n";
        }
    }

    auto run_under_profiling_ret(std::string const& context, auto&& func, std::source_location src = std::source_location::current())
    {
        auto start_time = std::chrono::steady_clock::now();
        auto result = std::forward< decltype(func) >(func)();
        auto end_time = std::chrono::steady_clock::now();

        auto duration = end_time - start_time;

        if (duration > profile_threshold)
        {
            std::cerr << "[quxlang:" << src.file_name() << ":" << src.line() << "] ";
            if (!context.empty())
            {
                std::cerr << "'" << context << "' ";
            }
            std::cerr << double(duration.count()) / 1000 / 1000 << "ms\n";
        }

        return result;
    }

    auto run_under_profiling_ret(auto&& func, std::source_location src = std::source_location::current())
    {
        return run_under_profiling_ret(std::string(), std::forward< decltype(func) >(func), src);
    }

    auto run_under_profiling_ret(std::function< std::string() > const& context, auto&& func, std::source_location src = std::source_location::current())
    {
        auto start_time = std::chrono::steady_clock::now();
        auto result = std::forward< decltype(func) >(func)();
        auto end_time = std::chrono::steady_clock::now();

        auto duration = end_time - start_time;

        if (duration > profile_threshold)
        {
            std::cerr << "[quxlang:" << src.file_name() << ":" << src.line() << "] ";
            auto const context_str = context();
            if (!context_str.empty())
            {
                std::cerr << "'" << context_str << "' ";
            }
            std::cerr << double(duration.count()) / 1000 / 1000 << "ms\n";
        }

        return result;
    }
}

#endif // QUXLANG_PROFILING_HPP
