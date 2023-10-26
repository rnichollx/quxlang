//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "rylang/collector.hpp"
#include "rylang/manipulators/expression_stringifier.hpp"
#include "rylang/manipulators/merge_entity.hpp"

#include <boost/variant.hpp>

struct foo
{
    int a;
};

struct bar
{
};

class collector_tester : public ::testing::Test
{
};

TEST_F(collector_tester, order_of_operations)
{
    rylang::collector c;

    std::string test_string = "a + b * c + d + e * f := g + h ^^ i * i * j";

    rylang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = c.collect_expression(it, it_end);

    std::string str = rylang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< rylang::expression_copy_assign >());
    ASSERT_EQ(it, it_end);
};

TEST(boost_assumptions, type_index)
{
    boost::variant< foo, boost::recursive_wrapper< bar > > v = bar{};

    ASSERT_EQ(boost::typeindex::type_id< bar >(), v.type());
}

TEST(rylang_modules, merge_entities)
{
    rylang::entity_ast a(rylang::class_entity_ast{}, false, {{"foo", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::entity_ast b(rylang::class_entity_ast{}, false, {{"bar", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::entity_ast c(rylang::class_entity_ast{}, false, {});

    rylang::entity_ast e(rylang::class_entity_ast{}, false, {{"foo", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}, {"bar", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    rylang::entity_ast e2(rylang::class_entity_ast{}, false, {{"foo", rylang::entity_ast{rylang::class_entity_ast{}, false, {}}}});

    ASSERT_NE(a, b);
    ASSERT_NE(a, c);
    ASSERT_NE(b, c);
    rylang::merge_entity(c, a);

    ASSERT_EQ(c, a);
    ASSERT_EQ(c, e2);
    ASSERT_NE(c, b);
    ASSERT_NE(c, e);

    rylang::merge_entity(c, b);

    ASSERT_EQ(c, e);
}
