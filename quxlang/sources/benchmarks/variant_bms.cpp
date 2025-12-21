// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL
#include "quxlang/data/expression.hpp"
#include "quxlang/parsers/parse_expression.hpp"

#include <benchmark/benchmark.h>


struct foo
{
    int x;
    int y;

    RPNX_MEMBER_METADATA(foo, x, y);
};

struct bar
{
    char a;
    char b;
    int x;

    RPNX_MEMBER_METADATA(bar, a, b, x);
};

struct baz
{
    int x;
    int y;
    int z;

    RPNX_MEMBER_METADATA(baz, x, y, z);
};

using foobarbaz = rpnx::variant< foo, bar, baz >;

static void BM_VariantIndirect(benchmark::State& state)
{
    std::vector<rpnx::variant<foo, bar, baz>> v;

    v.emplace_back(foo{1, 2});
    v.push_back(bar{'a', 'b', 3});
    v.push_back(baz{'c'});

    for (auto _ : state)
    {
        int q = 0;
        for (auto& item : v)
        {
            q = rpnx::apply_visitor<int, rpnx::dispatch_type::indirect>([](auto const & val) { return val.x; }, item);
            benchmark::DoNotOptimize(q);
        }
    }
}
BENCHMARK(BM_VariantIndirect);

static void BM_VariantBranching(benchmark::State& state)
{
    std::vector<rpnx::variant<foo, bar, baz>> v;

    v.emplace_back(foo{1, 2});
    v.push_back(bar{'a', 'b', 3});
    v.push_back(baz{'c'});

    for (auto _ : state)
    {
        int q = 0;
        for (auto& item : v)
        {
            q = rpnx::apply_visitor<int, rpnx::dispatch_type::branching>([](auto const & val) { return val.x; }, item);
            benchmark::DoNotOptimize(q);
        }
    }
}
BENCHMARK(BM_VariantBranching);

static void BM_SomeFunction(benchmark::State& state)
{
    std::vector< quxlang::expression > expressions;

    expressions.push_back(quxlang::parsers::parse_expression("a[& 999 + 42 * (foo.bar(123, baz) - 56)]"));

    expressions.push_back(quxlang::parsers::parse_expression("x.SERIALIZE( @OUTPUT_ITERATOR output[& 0] )"));

    expressions.push_back(quxlang::parsers::parse_expression("x := STATIC_CHOOSE( foolib::imported_function() == 42 , 10 , 20 )"));

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