// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL
#include <quxlang/data/basic_types.hpp>
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/parsers/parse_expression.hpp"

#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

template < std::size_t N, typename Int >
struct padded
{
    std::byte padding[N];
    Int x;

    RPNX_MEMBER_METADATA(padded, padding, x);
};

using variant_4 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t > >;

using variant_8 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t >, padded< 8, int32_t >, padded< 0, int8_t >, padded< 19, uint16_t >, padded< 6, int16_t > >;

using variant_16 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t >, padded< 8, int32_t >, padded< 0, int8_t >, padded< 19, uint16_t >, padded< 6, int16_t >, padded< 14, uint8_t >, padded< 23, uint32_t >, padded< 10, int32_t >, padded< 2, int8_t >, padded< 16, uint16_t >, padded< 5, int16_t >, padded< 13, uint8_t >, padded< 20, uint32_t > >;

using variant_32 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t >, padded< 8, int32_t >, padded< 0, int8_t >, padded< 19, uint16_t >, padded< 6, int16_t >, padded< 14, uint8_t >, padded< 23, uint32_t >, padded< 10, int32_t >, padded< 2, int8_t >, padded< 16, uint16_t >, padded< 5, int16_t >, padded< 13, uint8_t >, padded< 20, uint32_t >, padded< 9, int32_t >, padded< 1, int8_t >, padded< 18, uint16_t >, padded< 7, int16_t >, padded< 15, uint8_t >, padded< 22, uint32_t >, padded< 11, int32_t >, padded< 3, int8_t >, padded< 24, int64_t >, padded< 42, uint16_t >, padded< 7, int8_t >, padded< 68, int32_t >, padded< 15, int16_t >, padded< 33, uint8_t >, padded< 59, uint64_t >, padded< 24, int64_t > >;

using variant_64 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t >, padded< 8, int32_t >, padded< 0, int8_t >, padded< 19, uint16_t >, padded< 6, int16_t >, padded< 14, uint8_t >, padded< 23, uint32_t >, padded< 10, int32_t >, padded< 2, int8_t >, padded< 16, uint16_t >, padded< 5, int16_t >, padded< 13, uint8_t >, padded< 20, uint32_t >, padded< 9, int32_t >, padded< 1, int8_t >, padded< 18, uint16_t >, padded< 7, int16_t >, padded< 15, uint8_t >, padded< 22, uint32_t >, padded< 11, int32_t >, padded< 3, int8_t >, padded< 24, int64_t >, padded< 42, uint16_t >, padded< 7, int8_t >, padded< 68, int32_t >, padded< 15, int16_t >, padded< 33, uint8_t >, padded< 59, uint64_t >, padded< 24, int64_t >, padded< 71, int32_t >, padded< 2, int8_t >, padded< 46, uint16_t >, padded< 11, int16_t >, padded< 75, int32_t >, padded< 52, uint32_t >, padded< 28, int64_t >, padded< 39, uint8_t >, padded< 63, uint64_t >, padded< 18, int32_t >, padded< 4, int8_t >, padded< 40, uint16_t >, padded< 13, int16_t >, padded< 35, uint8_t >, padded< 61, uint64_t >, padded< 26, int64_t >, padded< 73, int32_t >, padded< 0, int8_t >, padded< 44, uint16_t >, padded< 9, int16_t >, padded< 77, int32_t >, padded< 50, uint32_t >, padded< 77, int64_t >, padded< 37, uint8_t >, padded< 65, int32_t >, padded< 21, int32_t >, padded< 6, int8_t >, padded< 42, uint16_t >, padded< 15, int16_t >, padded< 33, uint8_t >, padded< 59, uint64_t >, padded< 24, int64_t > >;

using variant_128 = rpnx::variant< padded< 17, uint16_t >, padded< 4, int16_t >, padded< 12, uint8_t >, padded< 21, uint32_t >, padded< 8, int32_t >, padded< 0, int8_t >, padded< 19, uint16_t >, padded< 6, int16_t >, padded< 14, uint8_t >, padded< 23, uint32_t >, padded< 10, int32_t >, padded< 2, int8_t >, padded< 16, uint16_t >, padded< 5, int16_t >, padded< 13, uint8_t >, padded< 20, uint32_t >, padded< 9, int32_t >, padded< 1, int8_t >, padded< 18, uint16_t >, padded< 7, int16_t >, padded< 15, uint8_t >, padded< 22, uint32_t >, padded< 11, int32_t >, padded< 3, int8_t >, padded< 24, int64_t >, padded< 42, uint16_t >, padded< 7, int8_t >, padded< 68, int32_t >, padded< 15, int16_t >, padded< 33, uint8_t >, padded< 59, uint64_t >, padded< 24, int64_t >, padded< 71, int32_t >, padded< 2, int8_t >, padded< 46, uint16_t >, padded< 11, int16_t >, padded< 75, int32_t >, padded< 52, uint32_t >, padded< 28, int64_t >, padded< 39, uint8_t >, padded< 63, uint64_t >, padded< 18, int32_t >, padded< 4, int8_t >, padded< 40, uint16_t >, padded< 13, int16_t >, padded< 35, uint8_t >, padded< 61, uint64_t >, padded< 26, int64_t >, padded< 73, int32_t >, padded< 0, int8_t >, padded< 44, uint16_t >, padded< 9, int16_t >, padded< 77, int32_t >, padded< 50, uint32_t >, padded< 77, int64_t >, padded< 37, uint8_t >, padded< 65, int32_t >, padded< 21, int32_t >, padded< 6, int8_t >, padded< 42, uint16_t >, padded< 15, int16_t >, padded< 33, uint8_t >, padded< 59, uint64_t >, padded< 24, int64_t >, padded< 71, int32_t >, padded< 3, int8_t >, padded< 47, uint16_t >, padded< 12, int16_t >, padded< 76, int32_t >, padded< 53, uint32_t >, padded< 29, int64_t >, padded< 40, uint8_t >, padded< 64, uint64_t >, padded< 19, int32_t >, padded< 5, int8_t >, padded< 41, uint16_t >, padded< 14, int16_t >, padded< 36, uint8_t >, padded< 62, uint64_t >, padded< 27, int64_t >, padded< 74, int32_t >, padded< 1, int8_t >, padded< 45, uint16_t >, padded< 10, int16_t >, padded< 78, int32_t >, padded< 51, uint32_t >, padded< 31, int64_t >, padded< 38, uint8_t >, padded< 66, int32_t >, padded< 22, int32_t >, padded< 17, int32_t >, padded< 43, uint16_t >, padded< 16, int16_t >, padded< 34, uint8_t >, padded< 60, uint64_t >, padded< 25, int64_t >, padded< 72, int32_t >, padded< 8, int16_t >, padded< 48, uint32_t >, padded< 32, uint8_t >, padded< 56, uint64_t >, padded< 20, int32_t >, padded< 54, uint32_t >, padded< 36, uint8_t >, padded< 79, int32_t >, padded< 81, uint64_t >, padded< 82, int8_t >, padded< 83, uint32_t >, padded< 84, int16_t >, padded< 85, uint8_t >, padded< 86, int64_t >, padded< 87, uint16_t >, padded< 88, int32_t >, padded< 89, int8_t >, padded< 90, uint32_t >, padded< 91, int16_t >, padded< 92, uint8_t >, padded< 93, int64_t >, padded< 94, uint16_t >, padded< 95, int32_t >, padded< 96, int8_t >, padded< 97, uint32_t >, padded< 98, int16_t >, padded< 99, uint8_t >, padded< 100, int64_t > >;

static constexpr std::size_t max_bm_size = 2097152 * 2;
;

template < typename Variant >
static std::vector< Variant > GenerateVariantArray(std::size_t count)
{
    std::vector< Variant > v;
    v.reserve(count);
    std::mt19937 gen(42);
    std::uniform_int_distribution< std::size_t > dis(0, rpnx::variant_size_v< Variant > - 1);
    std::uniform_int_distribution< int64_t > x_dis(0, 1000000);

    for (std::size_t i = 0; i < count; ++i)
    {
        std::size_t choice = dis(gen);
        int64_t x_val = x_dis(gen);

        // We use a helper function to construct the variant since it doesn't have emplace_index
        // and constructing the padded structs needs to handle the padding array and the value.
        // Since we can't easily construct the specific type from a index at runtime without a visitor,
        // we'll use apply_nth_visitor or similar if available, or just a switch for now since we know the types.

        // Actually, since all alternatives are padded<N, Int>, we can use a switch over the size of the variant.
        // But that's messy. Let's try to use the constructor that takes a value.
        // We need to construct the specific padded<N, Int> type.

        // Given the constraints and the variant API, the easiest way to populate it is to have a
        // function that returns a variant of the correct type.

        auto creator = [&]< std::size_t N >(std::integral_constant< std::size_t, N >)
        {
            using Type = rpnx::variant_nth_member_t< Variant, N >;
            using IntType = decltype(Type::x);
            v.emplace_back(Type{{}, static_cast< IntType >(x_val)});
        };

        // Custom simple runtime-to-compile-time index dispatch
        [&]< std::size_t... Is >(std::index_sequence< Is... >)
        {
            ((Is == choice ? creator(std::integral_constant< std::size_t, Is >{}) : void()), ...);
        }(std::make_index_sequence< rpnx::variant_size_v< Variant > >());
    }
    return v;
}

// Specializations for emplace because of the padding array.
// Actually, RPNX might need a better way to emplace into padded<N, Int>.
// Since padded<N, Int> is an aggregate, we might need a custom creator.

struct visitor_uint64
{
    template < typename T >
    uint64_t operator()(T const& val) const
    {
        return static_cast< uint64_t >(val.x);
    }
};

template < typename Variant >
static void RunBM_Variant(benchmark::State& state)
{
    auto v = GenerateVariantArray< Variant >(state.range(0));
    std::size_t i = 0;
    bool branched = state.range(2);
    if (branched)
    {
        for (auto _ : state)
        {
            uint64_t q = rpnx::apply_visitor< uint64_t, rpnx::dispatch_type::branching >(v[i], visitor_uint64{});
            benchmark::DoNotOptimize(q);
            if (++i == v.size()) [[unlikely]]
            {
                i = 0;
            }
        }
    }
    else
    {
        for (auto _ : state)
        {
            uint64_t q = rpnx::apply_visitor< uint64_t, rpnx::dispatch_type::indirect >(v[i], visitor_uint64{});
            benchmark::DoNotOptimize(q);
            if (++i == v.size()) [[unlikely]]
            {
                i = 0;
            }
        }
    }
}

static void BM_Variant(benchmark::State& state)
{
    int variations = state.range(1);
    switch (variations)
    {
    case 4:
        RunBM_Variant< variant_4 >(state);
        break;
    case 8:
        RunBM_Variant< variant_8 >(state);
        break;
    case 16:
        RunBM_Variant< variant_16 >(state);
        break;
    case 32:
        RunBM_Variant< variant_32 >(state);
        break;
    case 64:
        RunBM_Variant< variant_64 >(state);
        break;
    case 128:
        RunBM_Variant< variant_128 >(state);
        break;
    }
}

static void VariantArgs(benchmark::internal::Benchmark* b)
{
    b->ArgNames({"Object Count", "Variations", "Branched"});
    for (int size = 8; size <= max_bm_size; size *= 4)
    {
        for (int variations : {4, 8, 16, 32, 64, 128})
        {
            for (int branched : {1, 0})
            {
                b->Args({size, variations, branched});
            }
        }
    }
}

BENCHMARK(BM_Variant)->Apply(VariantArgs);

static void BM_SomeFunction(benchmark::State& state)
{
    auto parse_expression_text = [](std::string const& text) {
        auto ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        auto result = quxlang::parsers::parse_expression(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw std::logic_error("Input not fully parsed");
        }
        return result;
    };

    std::vector< quxlang::expression > expressions;

    expressions.push_back(parse_expression_text("a[& 999 + 42 * (foo.bar(123, baz) - 56)]"));

    expressions.push_back(parse_expression_text("x.SERIALIZE( @OUTPUT_ITERATOR output[& 0] )"));

    expressions.push_back(parse_expression_text("x := STATIC_CHOOSE( foolib::imported_function() == 42 , 10 , 20 )"));

    for (auto _ : state)
    {
        for (auto& expr : expressions)
        {
            auto str = quxlang::to_string(expr);
            benchmark::DoNotOptimize(str);
        }
    }
}

BENCHMARK(BM_SomeFunction);
