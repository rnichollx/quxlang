// Copyright (c) 2025 Ryan P. Nicholl $USER_EMAIL
#include "quxlang/data/expression.hpp"
#include "quxlang/parsers/parse_expression.hpp"

#include <benchmark/benchmark.h>

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