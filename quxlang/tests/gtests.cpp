//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "quxlang/collector.hpp"
#include "quxlang/manipulators/expression_stringifier.hpp"
#include "quxlang/manipulators/merge_entity.hpp"

#include "quxlang/cow.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include "quxlang/manipulators/mangler.hpp"

#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_symbol.hpp>
#include <quxlang/parsers/parse_whitespace.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

#include <boost/variant.hpp>
#include <quxlang/parsers/parse_file.hpp>
#include <quxlang/parsers/try_parse_class.hpp>

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

TEST(parsing, parse_empty_class)
{
    std::string test_string = "CLASS { }";

    std::optional< quxlang::ast2_class_declaration > cl = quxlang::parsers::try_parse_class(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, parse_class_with_variables)
{
    std::string test_string = "CLASS { .a VAR I32; .b VAR I64; ::c VAR I32; }";

    std::optional< quxlang::ast2_class_declaration > cl = quxlang::parsers::try_parse_class(test_string);

    ASSERT_TRUE(cl.has_value());

    auto cl2 = cl.value();

    ASSERT_EQ(cl2.globals.size(), 1);
    ASSERT_EQ(cl2.members.size(), 2);

    auto member1 = cl2.members[0];
    auto member2 = cl2.members[1];
    auto global = cl2.globals[0];

    std::pair< std::string, quxlang::ast2_declarable > member1_expected = {"a", quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{32, true})}};

    std::pair< std::string, quxlang::ast2_declarable > member2_expected = {"b", quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{64, true})}};

    std::pair< std::string, quxlang::ast2_declarable > global_expected = {"c", quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{32, true})}};

    ASSERT_TRUE(member1 == member1_expected);
    ASSERT_TRUE(member2 == member2_expected);
    ASSERT_TRUE(global == global_expected);
}

TEST(parsing, parse_class_constructor)
{
    std::string test_string = "CLASS { .CONSTRUCTOR FUNCTION(%a I32) { } }";

    auto cl = quxlang::parsers::try_parse_class(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, parse_class_constructor_delegates)
{
    std::string test_string = "CLASS { .CONSTRUCTOR FUNCTION(%a I32) :> .x:(1) { } }";

    auto cl = quxlang::parsers::try_parse_class(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, functum_combining)
{
    std::string test_string = "MODULE m; ::foo FUNCTION(%a I32) { } ::foo FUNCTION(%a I64) { }";

    quxlang::ast2_file_declaration file;
    file = quxlang::parsers::parse_file(test_string);

    ASSERT_EQ(file.globals.size(), 2);
    ASSERT_EQ(file.globals[0].first, "foo");
    ASSERT_EQ(file.globals[1].first, "foo");
}

TEST(parsing, parse_function_args)
{
    std::string test_string = "(%a I32, %b I64, %c -> I32)";

    auto args = quxlang::parsers::parse_function_args(test_string);

    ASSERT_EQ(args.size(), 3);
    ASSERT_EQ(args[0].name, "a");
    ASSERT_EQ(args[1].name, "b");
    ASSERT_EQ(args[2].name, "c");

    bool ok1 = args[0].type == quxlang::type_symbol(quxlang::primitive_type_integer_reference{32, true});
    bool ok2 = args[1].type == quxlang::type_symbol(quxlang::primitive_type_integer_reference{64, true});
    ASSERT_TRUE(ok1);
    ASSERT_TRUE(ok2);
}

TEST(parsing, parse_basic_types)
{
    using namespace quxlang::parsers;
    using namespace quxlang;

    ASSERT_TRUE(parse_type_symbol("I64") == type_symbol(primitive_type_integer_reference{64, true}));
    ASSERT_TRUE(parse_type_symbol("-> I64") == type_symbol(instance_pointer_type{primitive_type_integer_reference{64, true}}));

    ASSERT_TRUE(parse_type_symbol("BOOL") == type_symbol(primitive_type_bool_reference{}));
}

TEST(mangling, name_mangling_new)
{
    quxlang::module_reference module{"main"};

    quxlang::subentity_reference subentity{module, "foo"};

    quxlang::subentity_reference subentity2{subentity, "bar"};

    quxlang::subentity_reference subentity3{subentity2, "baz"};

    quxlang::instanciation_reference param_set{subentity3, {}};

    param_set.parameters.push_back(quxlang::primitive_type_integer_reference{32, true});
    param_set.parameters.push_back(quxlang::primitive_type_integer_reference{32, true});

    std::string mangled_name = quxlang::mangle(quxlang::type_symbol(param_set));

    ASSERT_EQ(mangled_name, "_S_MmainNfooNbarNbazCAI32AI32E");
}

TEST_F(collector_tester, order_of_operations)
{
    // quxlang::collector c;

    std::string test_string = "a + b * c + d + e * f := g + h ^^ i * j * k * l := m + n && o + p";

    quxlang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = quxlang::parsers::parse_expression(it, it_end);

    std::string str = quxlang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< quxlang::expression_binary >());
    ASSERT_TRUE(boost::get< quxlang::expression_binary >(expr).operator_str == ":=");
    ASSERT_EQ(it, it_end);
};

TEST(cow, cow_tests)
{

    quxlang::cow< int > a = 4;

    quxlang::cow< int > b = a;

    ASSERT_EQ(a, b);
    ASSERT_EQ(&a.get(), &b.get());

    a = 5;

    ASSERT_NE(a, b);
    ASSERT_EQ(a, 5);

    quxlang::cow< int > c = a;

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

    quxlang::cow< std::vector< quxlang::cow< std::string > > > strings = {};

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

    std::string test_string = "e.a.b(c, d, e.f)";

    quxlang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = quxlang::parsers::parse_expression(it, it_end);

    std::string str = quxlang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< quxlang::expression_call >());
    ASSERT_EQ(it, it_end);
};

TEST(boost_assumptions, type_index)
{
    boost::variant< foo, boost::recursive_wrapper< bar > > v = bar{};

    ASSERT_EQ(boost::typeindex::type_id< bar >(), v.type());
}

TEST(quxlang_modules, merge_entities)
{
    quxlang::entity_ast a(quxlang::class_entity_ast{}, false, {{"foo", quxlang::entity_ast{quxlang::class_entity_ast{}, false, {}}}});

    quxlang::entity_ast b(quxlang::class_entity_ast{}, false, {{"bar", quxlang::entity_ast{quxlang::class_entity_ast{}, false, {}}}});

    quxlang::entity_ast c(quxlang::class_entity_ast{}, false, {});

    quxlang::entity_ast e(quxlang::class_entity_ast{}, false, {{"foo", quxlang::entity_ast{quxlang::class_entity_ast{}, false, {}}}, {"bar", quxlang::entity_ast{quxlang::class_entity_ast{}, false, {}}}});

    quxlang::entity_ast e2(quxlang::class_entity_ast{}, false, {{"foo", quxlang::entity_ast{quxlang::class_entity_ast{}, false, {}}}});

    ASSERT_NE(a, b);
    ASSERT_NE(a, c);
    ASSERT_NE(b, c);
    quxlang::merge_entity(c, a);

    ASSERT_EQ(c, a);
    ASSERT_EQ(c, e2);
    ASSERT_NE(c, b);
    ASSERT_NE(c, e);

    quxlang::merge_entity(c, b);

    ASSERT_EQ(c, e);
}

TEST(qual, template_matching)
{
    quxlang::type_symbol template1 = quxlang::template_reference{"foo"};
    quxlang::type_symbol template2 = quxlang::instance_pointer_type{quxlang::template_reference{"foo"}};
    quxlang::type_symbol type1 = quxlang::primitive_type_integer_reference{32, true};
    quxlang::type_symbol type2 = quxlang::instance_pointer_type{quxlang::primitive_type_integer_reference{32, true}};

    auto res1 = quxlang::match_template(template1, type1);

    ASSERT_TRUE(res1.has_value());
    ASSERT_TRUE(res1.value().matches["foo"] == type1);
    ASSERT_TRUE(res1.value().matches["foo"] != type2);

    auto res2 = quxlang::match_template(template1, type2);

    ASSERT_TRUE(res2.has_value());
    ASSERT_TRUE(res2.value().matches["foo"] == type2);
    ASSERT_TRUE(res2.value().matches["foo"] != type1);

    auto res3 = quxlang::match_template(template2, type1);

    ASSERT_FALSE(res3.has_value());

    auto res4 = quxlang::match_template(template2, type2);
    ASSERT_TRUE(res4.has_value());
    ASSERT_TRUE(res4.value().matches["foo"] == type1);
    ASSERT_TRUE(res4.value().matches["foo"] != type2);
}
