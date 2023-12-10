//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "rylang/collector.hpp"
#include "rylang/manipulators/expression_stringifier.hpp"
#include "rylang/manipulators/merge_entity.hpp"

#include "rylang/cow.hpp"
#include "rylang/data/canonical_lookup_chain.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/manipulators/mangler.hpp"

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

TEST(mangling, name_mangling_new)
{
    rylang::module_reference module{"main"};

    rylang::subentity_reference subentity{module, "foo"};

    rylang::subentity_reference subentity2{subentity, "bar"};

    rylang::subentity_reference subentity3{subentity2, "baz"};

    rylang::functanoid_reference param_set{subentity3, {}, {}};

    param_set.parameters.push_back(rylang::primitive_type_integer_reference{32, true});
    param_set.parameters.push_back(rylang::primitive_type_integer_reference{32, true});

    std::string mangled_name = rylang::mangle(rylang::qualified_symbol_reference(param_set));

    ASSERT_EQ(mangled_name, "_S_MmainNfooNbarNbazCAI32AI32E");
}

TEST_F(collector_tester, order_of_operations)
{
    rylang::collector c;

    std::string test_string = "a + b * c + d + e * f := g + h ^^ i * j * k * l := m + n && o + p";

    rylang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = c.collect_expression(it, it_end);

    std::string str = rylang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< rylang::expression_binary >());
    ASSERT_TRUE(boost::get< rylang::expression_binary >(expr).operator_str == ":=");
    ASSERT_EQ(it, it_end);
};

TEST(cow, cow_tests)
{

    rylang::cow< int > a = 4;

    rylang::cow< int > b = a;

    ASSERT_EQ(a, b);
    ASSERT_EQ(&a.get(), &b.get());

    a = 5;

    ASSERT_NE(a, b);
    ASSERT_EQ(a, 5);

    rylang::cow< int > c = a;

    ASSERT_EQ(a, c);
    ASSERT_EQ(&a.get(), &c.get());
    a = 4;
    ASSERT_EQ(a, b);
    ASSERT_NE(&a.get(), &b.get());
    ASSERT_NE(a, c);

    c = a;
    ASSERT_EQ(a, c);
    ASSERT_EQ(&a.get(), &c.get());

    c.edit()++;
    ASSERT_NE(a, c);
    ASSERT_EQ(a, 4);
    ASSERT_EQ(c, 5);

    rylang::cow< std::vector< rylang::cow< std::string > > > strings = {};

    auto ptr1 = &strings.get();
    strings.edit().push_back("Hello");
    auto ptr2 = &strings.get();
    ASSERT_EQ(ptr1, ptr2);

    auto strings2 = strings;
    auto ptr3 = &strings2.get();
    ASSERT_EQ(ptr1, ptr3);

    strings2.edit().push_back("World");
    auto ptr4 = &strings2.get();
    ASSERT_NE(ptr3, ptr4);

    ASSERT_EQ(strings.get().size(), 1);
    ASSERT_EQ(strings2.get().size(), 2);

    ASSERT_EQ(strings->at(0), "Hello");
    ASSERT_EQ(strings2.get().at(0), "Hello");
    ASSERT_EQ(strings2.get().at(1), "World");

    ASSERT_EQ(strings->at(0), strings2->at(0));
    ASSERT_EQ(&strings->at(0).get(), &strings2->at(0).get());
    strings.edit().push_back("World");

    ASSERT_EQ(strings, strings2);
    ASSERT_EQ(strings->at(1), "World");
    ASSERT_EQ(strings->at(1), strings2->at(1));

    ASSERT_EQ(&strings->at(0).get(), &strings2->at(0).get());
    ASSERT_NE(&strings->at(1).get(), &strings2->at(1).get());
}

TEST_F(collector_tester, function_call)
{
    rylang::collector c;

    std::string test_string = "e.a.b(c, d, e.f)";

    rylang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = c.collect_expression(it, it_end);

    std::string str = rylang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< rylang::expression_call >());
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

TEST(qual, template_matching)
{
    rylang::qualified_symbol_reference template1 = rylang::template_reference{"foo"};
    rylang::qualified_symbol_reference template2 = rylang::instance_pointer_type{rylang::template_reference{"foo"}};
    rylang::qualified_symbol_reference type1 = rylang::primitive_type_integer_reference{32, true};
    rylang::qualified_symbol_reference type2 = rylang::instance_pointer_type{rylang::primitive_type_integer_reference{32, true}};

    auto res1 = rylang::match_template(template1, type1);

    ASSERT_TRUE(res1.has_value());
    ASSERT_TRUE(res1.value().matches["foo"] == type1);
    ASSERT_TRUE(res1.value().matches["foo"] != type2);

    auto res2 = rylang::match_template(template1, type2);

    ASSERT_TRUE(res2.has_value());
    ASSERT_TRUE(res2.value().matches["foo"] == type2);
    ASSERT_TRUE(res2.value().matches["foo"] != type1);

    auto res3 = rylang::match_template(template2, type1);

    ASSERT_FALSE(res3.has_value());

    auto res4 = rylang::match_template(template2, type2);
    ASSERT_TRUE(res4.has_value());
    ASSERT_TRUE(res4.value().matches["foo"] == type1);
    ASSERT_TRUE(res4.value().matches["foo"] != type2);


}
