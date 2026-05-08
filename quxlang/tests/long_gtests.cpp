#include <quxlang/fixed_bytemath.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <algorithm>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
    constexpr std::uint64_t default_random_value_count = std::uint64_t{1} << 32;

    struct mismatch
    {
        char const* operation = "";
        std::uint32_t lhs_bits = 0;
        std::uint32_t rhs_bits = 0;
        std::vector< std::byte > expected;
        std::vector< std::byte > actual;
    };

    using fixed_float_operation = quxlang::bytemath::float_result (*)(
        quxlang::bytemath::fixed_float_options,
        std::vector< std::byte >,
        std::vector< std::byte >);

    using native_float_operation = float (*)(float, float);

    struct arithmetic_case
    {
        char const* operation = "";
        std::uint32_t lhs_bits = 0;
        std::uint32_t rhs_bits = 0;
        fixed_float_operation fixed_operation = nullptr;
        native_float_operation native_operation = nullptr;
    };

    auto f32_options() -> quxlang::bytemath::fixed_float_options
    {
        return {.bits = 32, .exponent_bits = 8};
    }

    auto native_add(float a, float b) -> float
    {
        return a + b;
    }

    auto native_sub(float a, float b) -> float
    {
        return a - b;
    }

    auto native_mul(float a, float b) -> float
    {
        return a * b;
    }

    auto native_div(float a, float b) -> float
    {
        return a / b;
    }

    auto f32_bytes_from_bits(std::uint32_t bits) -> std::vector< std::byte >
    {
        return {
            std::byte{static_cast< unsigned char >(bits & 0xFF)},
            std::byte{static_cast< unsigned char >((bits >> 8) & 0xFF)},
            std::byte{static_cast< unsigned char >((bits >> 16) & 0xFF)},
            std::byte{static_cast< unsigned char >((bits >> 24) & 0xFF)},
        };
    }

    auto f32_bytes_from_native(float value) -> std::vector< std::byte >
    {
        return f32_bytes_from_bits(std::bit_cast< std::uint32_t >(value));
    }

    auto canonical_f32_bytes_from_native(float value) -> std::vector< std::byte >
    {
        auto opt = f32_options();
        if (std::isnan(value))
        {
            return quxlang::bytemath::fixed_float_nan(opt).data_bytes;
        }
        return f32_bytes_from_native(value);
    }

    auto native_from_f32_bits(std::uint32_t bits) -> float
    {
        return std::bit_cast< float >(bits);
    }

    auto bytes_to_hex(std::vector< std::byte > const& bytes) -> std::string
    {
        static constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(bytes.size() * 2);
        for (auto it = bytes.rbegin(); it != bytes.rend(); ++it)
        {
            auto value = std::to_integer< std::uint8_t >(*it);
            result.push_back(digits[value >> 4]);
            result.push_back(digits[value & 0x0F]);
        }
        return result;
    }

    template< typename FixedOp, typename NativeOp >
    auto find_mismatch(char const* operation, std::uint32_t lhs_bits, std::uint32_t rhs_bits, FixedOp fixed_operation, NativeOp native_operation) -> std::optional< mismatch >
    {
        auto opt = f32_options();
        auto lhs_bytes = f32_bytes_from_bits(lhs_bits);
        auto rhs_bytes = f32_bytes_from_bits(rhs_bits);
        auto fixed_result = fixed_operation(opt, lhs_bytes, rhs_bytes);
        if (fixed_result.result_is_undefined)
        {
            return mismatch{.operation = operation, .lhs_bits = lhs_bits, .rhs_bits = rhs_bits};
        }

        float lhs = native_from_f32_bits(lhs_bits);
        float rhs = native_from_f32_bits(rhs_bits);
        auto expected = canonical_f32_bytes_from_native(native_operation(lhs, rhs));
        if (fixed_result.data_bytes != expected)
        {
            return mismatch{
                .operation = operation,
                .lhs_bits = lhs_bits,
                .rhs_bits = rhs_bits,
                .expected = std::move(expected),
                .actual = std::move(fixed_result.data_bytes),
            };
        }

        return std::nullopt;
    }

    auto find_arithmetic_mismatch(std::uint32_t lhs_bits, std::uint32_t rhs_bits) -> std::optional< mismatch >
    {
        using namespace quxlang::bytemath;

        if (auto result = find_mismatch("add", lhs_bits, rhs_bits, fixed_float_add_le, native_add))
        {
            return result;
        }
        if (auto result = find_mismatch("sub", lhs_bits, rhs_bits, fixed_float_sub_le, native_sub))
        {
            return result;
        }
        if (auto result = find_mismatch("mul", lhs_bits, rhs_bits, fixed_float_mul_le, native_mul))
        {
            return result;
        }
        if (auto result = find_mismatch("div", lhs_bits, rhs_bits, fixed_float_div_le, native_div))
        {
            return result;
        }
        return std::nullopt;
    }

    auto find_mismatch(arithmetic_case const& test_case) -> std::optional< mismatch >
    {
        return find_mismatch(test_case.operation, test_case.lhs_bits, test_case.rhs_bits, test_case.fixed_operation, test_case.native_operation);
    }

    auto f32_edge_arithmetic_cases() -> std::vector< arithmetic_case >
    {
        using namespace quxlang::bytemath;

        return {
            {"add", 0x00000000u, 0x80000000u, fixed_float_add_le, native_add},
            {"sub", 0x00000000u, 0x80000000u, fixed_float_sub_le, native_sub},
            {"mul", 0x80000000u, 0xBF800000u, fixed_float_mul_le, native_mul},
            {"div", 0x80000000u, 0xBF800000u, fixed_float_div_le, native_div},

            {"add", 0x00000001u, 0x00000001u, fixed_float_add_le, native_add},
            {"sub", 0x00000001u, 0x00000001u, fixed_float_sub_le, native_sub},
            {"add", 0x007FFFFFu, 0x00000001u, fixed_float_add_le, native_add},
            {"mul", 0x00000001u, 0x40000000u, fixed_float_mul_le, native_mul},

            {"add", 0x7F800000u, 0xFF800000u, fixed_float_add_le, native_add},
            {"sub", 0xFF800000u, 0xFF800000u, fixed_float_sub_le, native_sub},
            {"mul", 0x7F800000u, 0x00000000u, fixed_float_mul_le, native_mul},
            {"div", 0x7F800000u, 0x7F800000u, fixed_float_div_le, native_div},
            {"div", 0x3F800000u, 0x00000000u, fixed_float_div_le, native_div},
            {"div", 0x3F800000u, 0x80000000u, fixed_float_div_le, native_div},
            {"div", 0x00000000u, 0x00000000u, fixed_float_div_le, native_div},

            {"mul", 0x7F7FFFFFu, 0x40000000u, fixed_float_mul_le, native_mul},
            {"sub", 0x7F7FFFFFu, 0x7F7FFFFFu, fixed_float_sub_le, native_sub},

            {"add", 0x7FC00000u, 0x3F800000u, fixed_float_add_le, native_add},
            {"add", 0xFFC00000u, 0x3F800000u, fixed_float_add_le, native_add},
            {"add", 0x7F800001u, 0x3F800000u, fixed_float_add_le, native_add},
            {"add", 0x7FFFFFFFu, 0x3F800000u, fixed_float_add_le, native_add},
        };
    }

    auto parse_random_value_count() -> std::uint64_t
    {
        char const* raw_value = std::getenv("QUXLANG_LONG_GTEST_F32_RANDOM_VALUES");
        if (raw_value == nullptr || raw_value[0] == '\0')
        {
            return default_random_value_count;
        }

        std::uint64_t result = 0;
        std::string_view text(raw_value);
        auto parse_result = std::from_chars(text.data(), text.data() + text.size(), result);
        if (parse_result.ec != std::errc{} || parse_result.ptr != text.data() + text.size())
        {
            throw quxlang::compilation_error("QUXLANG_LONG_GTEST_F32_RANDOM_VALUES must be an unsigned integer");
        }
        return result;
    }

    void check_no_mismatch(std::optional< mismatch > const& result)
    {
        ASSERT_FALSE(result.has_value())
            << result->operation << " mismatch"
            << " lhs=0x" << std::hex << result->lhs_bits
            << " rhs=0x" << std::hex << result->rhs_bits
            << " expected=0x" << bytes_to_hex(result->expected)
            << " actual=0x" << bytes_to_hex(result->actual);
    }

    void report_mismatch(mismatch const& result)
    {
        ADD_FAILURE()
            << result.operation << " mismatch"
            << " lhs=0x" << std::hex << result.lhs_bits
            << " rhs=0x" << std::hex << result.rhs_bits
            << " expected=0x" << bytes_to_hex(result.expected)
            << " actual=0x" << bytes_to_hex(result.actual);
    }
}

TEST(long_fixed_bytemath, f32_edge_arithmetic_matches_native_float)
{
    ASSERT_TRUE(std::numeric_limits< float >::is_iec559);

    for (auto const& test_case : f32_edge_arithmetic_cases())
    {
        check_no_mismatch(find_mismatch(test_case));
    }
}

TEST(long_fixed_bytemath, f32_random_arithmetic_matches_native_float)
{
    ASSERT_TRUE(std::numeric_limits< float >::is_iec559);

    std::uint64_t const value_count = parse_random_value_count();
    unsigned const hardware_threads = std::thread::hardware_concurrency();
    std::size_t const thread_count = std::max< std::size_t >(1, hardware_threads == 0 ? 1 : hardware_threads);
    std::uint64_t const pair_count = (value_count + 1) / 2;
    auto const edge_cases = f32_edge_arithmetic_cases();
    std::uint64_t const edge_pair_count = edge_cases.size();
    std::uint64_t const total_pair_count = pair_count + edge_pair_count;
    std::uint64_t const total_value_count = value_count + edge_pair_count * 2;
    std::atomic< bool > workers_done = false;
    std::atomic< bool > mismatch_found = false;
    std::mutex failure_report_mutex;
    std::mutex status_mutex;
    std::mutex status_output_mutex;
    std::condition_variable status_cv;
    std::vector< std::atomic< std::uint64_t > > completed_pairs_by_thread(thread_count);
    std::vector< std::thread > threads;
    threads.reserve(thread_count);
    auto const start_time = std::chrono::steady_clock::now();

    auto count_completed_pairs = [&]() -> std::uint64_t
    {
        std::uint64_t result = 0;
        for (auto const& completed_pairs : completed_pairs_by_thread)
        {
            result += completed_pairs.load(std::memory_order_relaxed);
        }
        return result;
    };

    auto print_status = [&](char const* status)
    {
        auto const now = std::chrono::steady_clock::now();
        auto const elapsed = std::chrono::duration< double >(now - start_time).count();
        auto const completed_pairs = std::min(count_completed_pairs(), total_pair_count);
        auto const completed_values = std::min(total_value_count, completed_pairs * 2);
        auto const rate = elapsed == 0.0 ? 0.0 : static_cast< double >(completed_values) / elapsed;

        std::lock_guard lock(status_output_mutex);
        std::cout << "quxlang_long_gtests f32 random " << status << ": "
                  << completed_values << "/" << total_value_count
                  << " values, " << completed_pairs << "/" << total_pair_count
                  << " pairs, elapsed=" << elapsed
                  << "s, rate=" << rate << " values/s"
                  << std::endl;
    };

    std::thread status_thread([&]() {
        std::unique_lock lock(status_mutex);
        while (!workers_done.load(std::memory_order_relaxed))
        {
            if (status_cv.wait_for(lock, std::chrono::seconds(5), [&]() { return workers_done.load(std::memory_order_relaxed); }))
            {
                break;
            }

            lock.unlock();
            print_status("status");
            lock.lock();
        }
    });

    for (std::size_t thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        std::uint64_t const begin = (pair_count * thread_index) / thread_count;
        std::uint64_t const end = (pair_count * (thread_index + 1)) / thread_count;

        threads.emplace_back([&, thread_index, begin, end]() {
            std::seed_seq seed{
                0x51554C58u,
                0x464C5432u,
                static_cast< std::uint32_t >(thread_index),
                static_cast< std::uint32_t >(thread_index >> 32),
            };
            std::mt19937 generator(seed);

            std::uint64_t processed_pairs = 0;
            for (std::size_t edge_index = thread_index; edge_index < edge_cases.size(); edge_index += thread_count)
            {
                if (auto result = find_mismatch(edge_cases.at(edge_index)))
                {
                    completed_pairs_by_thread[thread_index].store(processed_pairs + 1, std::memory_order_relaxed);
                    mismatch_found.store(true, std::memory_order_relaxed);
                    std::lock_guard lock(failure_report_mutex);
                    report_mismatch(*result);
                }

                ++processed_pairs;
            }

            for (std::uint64_t i = begin; i < end; ++i)
            {
                std::uint32_t lhs = generator();
                std::uint32_t rhs = generator();
                if (auto result = find_arithmetic_mismatch(lhs, rhs))
                {
                    completed_pairs_by_thread[thread_index].store(processed_pairs + 1, std::memory_order_relaxed);
                    mismatch_found.store(true, std::memory_order_relaxed);
                    std::lock_guard lock(failure_report_mutex);
                    report_mismatch(*result);
                }

                ++processed_pairs;
                if ((processed_pairs & 0xFFFu) == 0)
                {
                    completed_pairs_by_thread[thread_index].store(processed_pairs, std::memory_order_relaxed);
                }
            }

            completed_pairs_by_thread[thread_index].store(processed_pairs, std::memory_order_relaxed);
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    workers_done.store(true, std::memory_order_relaxed);
    status_cv.notify_one();
    status_thread.join();
    print_status("complete");

    EXPECT_FALSE(mismatch_found.load(std::memory_order_relaxed));
}
