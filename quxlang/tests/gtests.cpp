//
// Created by Ryan Nicholl on 10/6/23.
//

#include <gtest/gtest.h>

#include "quxlang/manipulators/expression_stringifier.hpp"
#include "quxlang/manipulators/merge_entity.hpp"

#include "quxlang/cow.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "rpnx/range.hpp"

#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_symbol.hpp>
#include <quxlang/parsers/parse_whitespace.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

#include <boost/variant.hpp>
#include <quxlang/parsers/parse_file.hpp>
#include <quxlang/parsers/try_parse_class.hpp>
#include <quxlang/vmir2/assembly.hpp>

#include "rpnx/serializer.hpp"

struct foo
{
    int a;
};

struct bar
{
};
/*
class collector_tester : public ::testing::Test
{
};
 */

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

    ASSERT_EQ(cl2.declarations.size(), 3);

    auto member1 = cl2.declarations[0];
    auto member2 = cl2.declarations[1];
    auto global = cl2.declarations[2];

    quxlang::subdeclaroid member1_expected = quxlang::member_subdeclaroid{.name = "a", .decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{32, true})}};

    quxlang::subdeclaroid member2_expected = quxlang::member_subdeclaroid{.name = "b", .decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{64, true})}};

    quxlang::subdeclaroid global_expected = quxlang::global_subdeclaroid{.name = "c", .decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::primitive_type_integer_reference{32, true})}};

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

    ASSERT_EQ(file.declarations.size(), 2);

    // ASSERT_EQ(file.globals[1].first, "foo");
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

    param_set.parameters.positional_parameters.push_back(quxlang::primitive_type_integer_reference{32, true});
    param_set.parameters.positional_parameters.push_back(quxlang::primitive_type_integer_reference{32, true});

    std::string mangled_name = quxlang::mangle(quxlang::type_symbol(param_set));

    ASSERT_EQ(mangled_name, "_S_MmainNfooNbarNbazCAPI32API32E");
}

TEST(collector_tester, order_of_operations)
{
    // quxlang::collector c;

    std::string test_string = "a + b * c + d + e * f := g + h ^^ i * j * k * l := m + n && o + p";

    quxlang::expression expr;

    std::string::iterator it = test_string.begin();
    std::string::iterator it_end = test_string.end();

    expr = quxlang::parsers::parse_expression(it, it_end);

    std::string str = quxlang::to_string(expr);

    ASSERT_TRUE(expr.type() == boost::typeindex::type_id< quxlang::expression_binary >());
    ASSERT_TRUE(as< quxlang::expression_binary >(expr).operator_str == ":=");
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

TEST(collector_tester, function_call)
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
    // TODO: Needs rewrite with the replaced merger
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

TEST(range, range_input)
{
    std::vector< std::byte > v = {std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
    rpnx::dyn_input_range< std::byte > range(v.begin(), v.end());

    std::vector< std::byte > v2;

    rpnx::dyn_output_iter< std::byte > out(std::back_inserter(v2));

    for (auto x : range)
    {
        *out++ = x;
    }

    ASSERT_EQ(v, v2);
    v2.push_back(std::byte(5));
    ASSERT_NE(v, v2);
}

TEST(variant, variant_meta)
{
    rpnx::variant< int, std::string > v = 5;
    ASSERT_TRUE(v.index() == 0);
    ASSERT_TRUE(v.get_as< int >() == 5);
    v = std::string("hello");
    ASSERT_TRUE(v.index() == 1);
    ASSERT_TRUE(v.get_as< std::string >() == "hello");

    rpnx::variant< int, std::string > v2;

    // ASSERT_THROW(v2.get_as<std::string>());
    ASSERT_TRUE(v2 < v);

    std::pair< int, rpnx::variant< int, std::string > > p = {5, std::string("hello")};

    std::map< rpnx::variant< int, std::string >, int > mp;
    mp[v2] = 9;

    v2 = 5;
    mp[v2] = 6;
    ASSERT_TRUE(mp[0] == 9);
    ASSERT_TRUE(mp[5] == 6);
    ASSERT_TRUE(mp[5] != 7);
    // ASSERT_TRUE(mp[5] == 7);
}

TEST(range, iterator_copy_constructor_and_assignment)
{
    std::vector< int > v = {1, 2, 3, 4};
    rpnx::dyn_input_iter< int > iter1(v.begin());
    rpnx::dyn_input_iter< int > iter2(iter1);
    ASSERT_EQ(*iter1, *iter2);

    ++iter1;
    ASSERT_NE(*iter1, *iter2);

    iter2 = iter1;
    ASSERT_EQ(*iter1, *iter2);
}

TEST(range, iterator_comparison)
{
    std::vector< int > v = {1, 2, 3, 4};
    rpnx::dyn_comparable_input_iter< int > iter1(v.begin());
    rpnx::dyn_comparable_input_iter< int > iter2(v.begin() + 1);
    ASSERT_TRUE(iter1 < iter2);
    ASSERT_FALSE(iter2 < iter1);
    ASSERT_TRUE(iter1 != iter2);
    ASSERT_FALSE(iter1 == iter2);

    ++iter1;
    ASSERT_FALSE(iter1 < iter2);
    ASSERT_FALSE(iter2 < iter1);
    ASSERT_TRUE(iter1 == iter2);
    ASSERT_FALSE(iter1 != iter2);
}

TEST(range, iterator_advance)
{
    std::vector< int > v = {1, 2, 3, 4};
    rpnx::dyn_input_iter< int > iter(v.begin());
    ASSERT_EQ(*iter, 1);

    ++iter;
    ASSERT_EQ(*iter, 2);

    iter++;
    ASSERT_EQ(*iter, 3);
}

#include "expr_test_provider.hpp"
#include "quxlang/compiler.hpp"
#include "rpnx/range.hpp"
#include <gtest/gtest.h>
#include <list>
#include <string>
#include <vector>

TEST(dyn_bidirectional_input_iter, construct_comparison)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.begin());
    EXPECT_EQ(*iter, 1);
}

TEST(dyn_bidirectional_input_iter, copy)
{
    std::list< std::string > lst = {"Hello", "World"};
    rpnx::dyn_bidirectional_input_iter< std::string > iter1(lst.begin());
    rpnx::dyn_bidirectional_input_iter< std::string > iter2(iter1);
    EXPECT_EQ(*iter1, "Hello");
    EXPECT_EQ(*iter2, "Hello");
}

TEST(dyn_bidirectional_input_iter, dereference)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.begin());
    EXPECT_EQ(*iter, 1);
}

TEST(dyn_bidirectional_input_iter, preincrement)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.begin());
    EXPECT_EQ(*iter, 1);
    ++iter;
    EXPECT_EQ(*iter, 2);
}

TEST(dyn_bidirectional_input_iter, postincrement)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.begin());
    EXPECT_EQ(*iter, 1);
    rpnx::dyn_bidirectional_input_iter< int > iter2 = iter++;
    EXPECT_EQ(*iter, 2);
    EXPECT_EQ(*iter2, 1);
}

TEST(dyn_bidirectional_input_iter, predecrement)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.end());
    --iter;
    EXPECT_EQ(*iter, 5);
}

TEST(dyn_bidirectional_input_iter, postdecrement)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter(vec.end());
    rpnx::dyn_bidirectional_input_iter< int > iter2 = iter--;
    EXPECT_EQ(*iter, 5);
    EXPECT_EQ(iter2, vec.end());
    --iter2;
    EXPECT_EQ(*--iter2, 4);
}

TEST(dyn_bidirectional_input_iter, noteq)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter1(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter2(vec.begin() + 2);
    EXPECT_TRUE(iter1 != iter2);
}

TEST(dyn_bidirectional_input_iter, eq)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter1(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter2(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter3(vec.begin() + 1);
    EXPECT_TRUE(iter1 == iter2);
    EXPECT_FALSE(iter1 == iter3);
}

TEST(dyn_bidirectional_input_iter, inequality_comparison)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter1(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter2(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter3(vec.begin() + 1);
    EXPECT_FALSE(iter1 != iter2);
    EXPECT_TRUE(iter1 != iter3);
}

TEST(dyn_bidirectional_input_iter, iterators_with_different_underlying_types)
{
    std::vector< int > vec = {1, 2, 3, 4, 5};
    std::list< int > lst = {1, 2, 3, 4, 5};
    rpnx::dyn_bidirectional_input_iter< int > iter1(vec.begin());
    rpnx::dyn_bidirectional_input_iter< int > iter2(lst.begin());
    EXPECT_FALSE(iter1 == iter2);
    ++iter1;
    ++iter2;
    EXPECT_FALSE(iter1 == iter2);
    EXPECT_TRUE(*iter1 == *iter2);
}

TEST(dyn_bidirectional_input_iter, IteratorsWithNonDefaultConstructibleType)
{
    struct NonDefaultConstructible
    {
        NonDefaultConstructible() = delete;

        explicit NonDefaultConstructible(int x)
            : value(x)
        {
        }

        int value;
    };

    std::vector< NonDefaultConstructible > vec;
    vec.emplace_back(1);
    vec.emplace_back(2);
    vec.emplace_back(3);

    rpnx::dyn_bidirectional_input_iter< NonDefaultConstructible > iter(vec.begin());
    EXPECT_EQ((*iter).value, 1);
    ++iter;
    EXPECT_EQ((*iter).value, 2);
}

TEST(SerializationTest, IntegralTypes)
{
    std::vector< std::byte > buffer;

    // Serialize integral types
    std::uint32_t uint32_value = 0x12345678;
    std::int64_t int64_value = -0x1234567890ABCDEF;

    rpnx::serialize_iter(uint32_value, std::back_inserter(buffer));
    rpnx::serialize_iter(int64_value, std::back_inserter(buffer));

    // Deserialize integral types
    auto it = buffer.begin();
    std::uint32_t deserialized_uint32;
    std::int64_t deserialized_int64;

    it = rpnx::deserialize_iter(deserialized_uint32, it);
    it = rpnx::deserialize_iter(deserialized_int64, it);

    EXPECT_EQ(uint32_value, deserialized_uint32);
    EXPECT_EQ(int64_value, deserialized_int64);
}

TEST(SerializationTest, Map)
{
    std::vector< std::byte > buffer;

    // Serialize map
    std::map< std::uint32_t, std::string > map = {{1, "one"}, {2, "two"}, {3, "three"}};

    rpnx::serialize_iter(map, std::back_inserter(buffer));

    // Deserialize map
    std::map< std::uint32_t, std::string > deserialized_map;
    auto it = buffer.begin();

    it = rpnx::deserialize_iter(deserialized_map, it);

    EXPECT_EQ(map, deserialized_map);
}

TEST(SerializationTest, Vector)
{
    std::vector< std::byte > buffer;

    // Serialize vector
    std::vector< std::uint32_t > vector = {1, 2, 3, 4, 5};

    rpnx::serialize_iter(vector, std::back_inserter(buffer));

    // Deserialize vector
    std::vector< std::uint32_t > deserialized_vector;
    auto it = buffer.begin();

    it = rpnx::deserialize_iter(deserialized_vector, it);

    EXPECT_EQ(vector, deserialized_vector);
}

TEST(SerializationTest, Set)
{
    std::vector< std::byte > buffer;

    // Serialize set
    std::set< std::string > set = {"one", "two", "three"};

    rpnx::serialize_iter(set, std::back_inserter(buffer));

    // Deserialize set
    std::set< std::string > deserialized_set;
    auto it = buffer.begin();

    it = rpnx::deserialize_iter(deserialized_set, it);

    EXPECT_EQ(set, deserialized_set);
}

TEST(SerializerTest, TupleSerializationDeserialization)
{
    std::tuple< int, std::string, int > original{42, "hello", 3};

    // Serialization
    std::vector< std::byte > bytes;
    rpnx::serialize_iter(original, std::back_inserter(bytes));

    // Deserialization
    std::tuple< int, std::string, int > deserialized;
    auto it = rpnx::deserialize_iter(deserialized, bytes.begin(), bytes.end());
    EXPECT_EQ(it, bytes.end());

    // Check equality of original and deserialized tuples
    EXPECT_EQ(original, deserialized);
}

TEST(SerializerTest, TieSerializationDeserialization)
{
    int a = 42;
    std::string b = "hello";
    int c = 3;

    // Serialization
    std::vector< std::byte > bytes;
    rpnx::serialize_iter(std::tie(a, b, c), std::back_inserter(bytes));

    // Deserialization
    int a_deserialized;
    std::string b_deserialized;
    int c_deserialized;
    auto it = rpnx::deserialize_iter(std::tie(a_deserialized, b_deserialized, c_deserialized), bytes.begin(), bytes.end());
    EXPECT_EQ(it, bytes.end());

    // Check equality of original and deserialized values
    EXPECT_EQ(a, a_deserialized);
    EXPECT_EQ(b, b_deserialized);
    EXPECT_EQ(c, c_deserialized);
}

TEST(SerializerTest, EmptyTuple)
{
    std::tuple<> original;

    // Serialization
    std::vector< std::byte > bytes;
    rpnx::serialize_iter(original, std::back_inserter(bytes));

    // Deserialization
    std::tuple<> deserialized;
    auto it = rpnx::deserialize_iter(deserialized, bytes.begin(), bytes.end());
    EXPECT_EQ(it, bytes.end());

    // Check equality of original and deserialized tuples
    EXPECT_EQ(original, deserialized);
}

TEST(VariantTest, Serialization)
{
    rpnx::variant< int, std::string > v1(42);
    rpnx::variant< int, std::string > v2(std::string("hello"));

    std::vector< std::byte > buffer;
    std::vector< std::byte > buffer2;
    rpnx::serialize_iter(v1, std::back_inserter(buffer));
    rpnx::serialize_iter(v2, std::back_inserter(buffer2));

    rpnx::variant< int, std::string > v3;
    rpnx::variant< int, std::string > v4;
    rpnx::deserialize_iter(v3, buffer.cbegin(), buffer.cend());
    rpnx::deserialize_iter(v4, buffer2.cbegin(), buffer2.cend());

    EXPECT_EQ(v1, v3);
    EXPECT_EQ(v2, v4);

    EXPECT_EQ(v4, std::string("hello"));
}

TEST(expression_ir, generation)
{
    quxlang::expr_test_provider pv;
    quxlang::source_bundle sources;
    sources.targets["foo"].target_output_config = quxlang::output_info{
        .cpu_type = quxlang::cpu::x86_64,
        .os_type = quxlang::os::linux,
        .binary_type = quxlang::binary::elf,
    };
    quxlang::compiler c(sources, "foo");
    quxlang::expr_test_provider::interface pvi(&pv);
    quxlang::co_vmir_expression_emitter em(&c, &pvi);

    // TODO: We could probably make the tester look at named variable slots instead of having a lookup table
    pv.slots.push_back(quxlang::vmir2::vm_slot{
        .type = quxlang::parsers::parse_type_symbol("I32"),
        .name = "a",
    });
    pv.slots.push_back(quxlang::vmir2::vm_slot{
        .type = quxlang::parsers::parse_type_symbol("I32"),
        .name = "b",
    });

    pv.loadable_symbols[quxlang::parsers::parse_type_symbol("a")] = 1;
    pv.loadable_symbols[quxlang::parsers::parse_type_symbol("b")] = 2;

    quxlang::expression expr = quxlang::parsers::parse_expression("a + b - 4");

    em.generate_expr(expr);

    quxlang::vmir2::functanoid_routine r;
    r.slots = pv.slots;
    r.instructions = pv.instructions;

    std::string result = quxlang::vmir2::assembler().to_string(r);

    std::cout << result << std::endl;
}