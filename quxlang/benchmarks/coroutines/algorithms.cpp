// Copyright 2026 Ryan P. Nicholl
#include "runtime.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace coroutine_benchmark
{
    /** Produces deterministic input with unsigned modular arithmetic. */
    std::uint64_t mix(std::uint64_t value)
    {
        value += UINT64_C(11400714819323198485);
        value = (value ^ (value >> 30)) * UINT64_C(13787848793156543929);
        value = (value ^ (value >> 27)) * UINT64_C(10723151780598845931);
        return value ^ (value >> 31);
    }

    /** Partitions a half-open range around its middle element. */
    std::size_t partition(std::vector<std::uint64_t>& values, std::size_t first, std::size_t last)
    {
        std::swap(values[first + (last - first) / 2], values[last - 1]);
        std::uint64_t pivot = values[last - 1];
        std::size_t destination = first;
        for (std::size_t index = first; index + 1 < last; ++index)
            if (values[index] < pivot) std::swap(values[destination++], values[index]);
        std::swap(values[destination], values[last - 1]);
        return destination;
    }

    /** Sorts by recursive partitioning down to single-element ranges. */
    void sort_serial(std::vector<std::uint64_t>& values, std::size_t first, std::size_t last)
    {
        if (last - first <= 1)
        {
            return;
        }
        std::size_t pivot = partition(values, first, last);
        sort_serial(values, first, pivot);
        sort_serial(values, pivot + 1, last);
    }

    /** Invokes both partitions as child coroutines on the calling thread. */
    task sort_coroutine(std::vector<std::uint64_t>& values, std::size_t first, std::size_t last, std::size_t cutoff, bool suspend)
    {
        if (suspend) co_await std::suspend_always{};
        if (last - first <= cutoff) { sort_serial(values, first, last); co_return 0; }
        std::size_t pivot = partition(values, first, last);
        co_await sort_coroutine(values, first, pivot, cutoff, suspend);
        co_await sort_coroutine(values, pivot + 1, last, cutoff, suspend);
        co_return 0;
    }

    /** Reduces hashed indices with the same balanced splitting used by coroutine tasks. */
    std::uint64_t reduce_serial(std::size_t first, std::size_t last, std::size_t cutoff, std::uint64_t seed)
    {
        if (last - first <= cutoff)
        {
            std::uint64_t total = 0;
            for (std::size_t index = first; index < last; ++index) total += mix(seed + index);
            return total;
        }
        std::size_t middle = first + (last - first) / 2;
        return reduce_serial(first, middle, cutoff, seed) + reduce_serial(middle, last, cutoff, seed);
    }

    /** Reduces a balanced tree through nested child coroutine invocations. */
    task reduce_coroutine(std::size_t first, std::size_t last, std::size_t cutoff, std::uint64_t seed, bool suspend)
    {
        if (suspend) co_await std::suspend_always{};
        if (last - first <= cutoff) co_return reduce_serial(first, last, cutoff, seed);
        std::size_t middle = first + (last - first) / 2;
        std::uint64_t total = co_await reduce_coroutine(first, middle, cutoff, seed, suspend);
        co_return total + co_await reduce_coroutine(middle, last, cutoff, seed, suspend);
    }

    /** Counts all completions of a bitboard N-queens search state. */
    std::uint64_t queens_serial(std::uint64_t mask, std::uint64_t columns, std::uint64_t left, std::uint64_t right)
    {
        if (columns == mask) return 1;
        std::uint64_t available = mask & ~(columns | left | right);
        std::uint64_t total = 0;
        while (available)
        {
            std::uint64_t bit = available & (std::uint64_t(0) - available);
            available -= bit;
            total += queens_serial(mask, columns | bit, (left | bit) << 1, (right | bit) >> 1);
        }
        return total;
    }

    /** Visits search branches through nested child coroutine invocations. */
    task queens_coroutine(std::uint64_t mask, std::uint64_t columns, std::uint64_t left, std::uint64_t right, std::size_t depth, bool suspend)
    {
        if (suspend) co_await std::suspend_always{};
        if (depth == 0 || columns == mask) co_return queens_serial(mask, columns, left, right);
        std::uint64_t available = mask & ~(columns | left | right);
        std::uint64_t total = 0;
        while (available)
        {
            std::uint64_t bit = available & (std::uint64_t(0) - available);
            available -= bit;
            total += co_await queens_coroutine(mask, columns | bit, (left | bit) << 1, (right | bit) >> 1, depth - 1, suspend);
        }
        co_return total;
    }
}

/** Validates arguments, times algorithm execution, and prints elapsed nanoseconds and a checksum. */
int main(int argc, char** argv)
{
    if (argc != 6) return 2;
    try
    {
        std::size_t algorithm = std::stoull(argv[1]);
        std::size_t n = std::stoull(argv[2]);
        std::size_t cutoff = std::stoull(argv[3]);
        std::size_t mode = std::stoull(argv[4]);
        std::size_t passes = std::stoull(argv[5]);
        if (algorithm > 2 || !n || !cutoff || mode > 2 || !passes ||
            (algorithm == 2 ? n > 14 || cutoff > n : n > 4194304)) return 2;
        std::vector<std::uint64_t> values(algorithm == 0 ? n : 0);
        std::atomic<std::size_t> input_size(n);
        std::uint64_t elapsed = 0;
        std::uint64_t checksum = 0;
        for (std::size_t pass = 0; pass < passes; ++pass)
        {
            std::size_t size = input_size.load(std::memory_order_relaxed);
            std::uint64_t seed = 20261104 + pass % 8;
            for (std::size_t index = 0; index < values.size(); ++index) values[index] = coroutine_benchmark::mix(seed + index);
            std::uint64_t result = 0;
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            if (algorithm == 0)
            {
                if (mode) coroutine_benchmark::sort_coroutine(values, 0, size, cutoff, mode == 2).get();
                else coroutine_benchmark::sort_serial(values, 0, size);
            }
            else if (algorithm == 1)
            {
                if (mode) result = coroutine_benchmark::reduce_coroutine(0, size, cutoff, seed, mode == 2).get();
                else result = coroutine_benchmark::reduce_serial(0, size, cutoff, seed);
            }
            else
            {
                std::uint64_t mask = (UINT64_C(1) << size) - 1;
                if (mode) result = coroutine_benchmark::queens_coroutine(mask, 0, 0, 0, cutoff, mode == 2).get();
                else result = coroutine_benchmark::queens_serial(mask, 0, 0, 0);
            }
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            elapsed += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
            if (algorithm == 0)
            {
                if (!std::is_sorted(values.begin(), values.end())) throw std::runtime_error("unsorted output");
                for (std::size_t index = 0; index < size; ++index) result += values[index] * (index + 1);
            }
            checksum += result;
        }
        std::printf("%" PRIu64 " %" PRIu64 "\n", elapsed, checksum);
    }
    catch (std::exception const& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
