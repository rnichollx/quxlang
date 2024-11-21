// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
// #define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_MAIN
#include <string>

#include "catch2/catch2.hpp"
#include "include/rpnx/resolver_utilities.hpp"
#include <iostream>
#include <rpnx/variant.hpp>

class test_graph;

class fibbonachi_resolver : public rpnx::resolver_base< test_graph, std::int64_t >
{
public:
    using key_type = std::int64_t;
    using value_type = std::int64_t;

private:
    key_type m_value;

public:
    fibbonachi_resolver(key_type value)
        : m_value(value)
    {
    }

    virtual ~fibbonachi_resolver()
    {
    }

    void process(test_graph* graph);
};

class unsolvably_recursive_resolver : public rpnx::resolver_base< test_graph, std::int64_t >
{
public:
    using key_type = std::int64_t;
    using value_type = std::int64_t;

private:
    key_type m_value;

public:
    unsolvably_recursive_resolver(key_type value)
        : m_value(value)
    {
    }

    virtual ~unsolvably_recursive_resolver()
    {
    }

    void process(test_graph* graph);
};

class test_graph
{
    rpnx::index< test_graph, fibbonachi_resolver > m_fib;
    rpnx::index< test_graph, unsolvably_recursive_resolver > m_rec;

public:
    rpnx::output_ptr< test_graph, std::int64_t > fib(std::int64_t n)
    {
        return m_fib.lookup(n);
    }

    auto rec(std::int64_t n)
    {
        return m_rec.lookup(n);
    }
};

void fibbonachi_resolver::process(test_graph* graph)
{
    if (m_value == 0)
    {
        set_value(0);
        return;
    }
    if (m_value <= 2)
    {
        set_value(1);
        return;
    }

    auto a = get_dependency(
        [&]
        {
            return graph->fib(m_value - 1);
        });
    auto b = get_dependency(
        [&]
        {
            return graph->fib(m_value - 2);
        });
    if (!ready())
        return;

    auto a_value = a->get();
    auto b_value = b->get();

    set_value(a_value + b_value);
}

void unsolvably_recursive_resolver::process(test_graph* graph)
{
    if (m_value == 1)
    {
        add_dependency(graph->rec(-1));
        return;
    }

    if (m_value == -1)
    {
        add_dependency(graph->rec(7));
        return;
    }
    if (m_value == 7)
    {
        add_dependency(graph->rec(1));
        return;
    }
    set_value(0);
}

TEST_CASE("solver", "[graph_solver]")
{
    test_graph g;
    rpnx::single_thread_graph_solver< test_graph > solver;
    auto f = g.fib(10);
    solver.solve(&g, f);
    REQUIRE(f->get() == 55);
}

TEST_CASE("unsolvable_issue", "[graph_solver]")
{
    test_graph g;
    rpnx::single_thread_graph_solver< test_graph > solver;
    auto f = g.rec(7);
    REQUIRE_THROWS(solver.solve(&g, f));
}

