// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include "quxlang/manipulators/expression_stringifier.hpp"
#include "quxlang/manipulators/merge_entity.hpp"

#include "quxlang/cow.hpp"
#include "quxlang/exception.hpp"
#include "quxlang/manipulators/mangler.hpp"


#include <quxlang/data/basic_types.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/parsers/parse_symbol.hpp>
#include <quxlang/parsers/statements.hpp>
#include <quxlang/parsers/parse_whitespace.hpp>
#include <quxlang/parsers/try_parse_expression.hpp>

#include <quxlang/parsers/parse_file.hpp>
#include <quxlang/parsers/try_parse_class.hpp>
#include <quxlang/vmir2/assembler.hpp>
#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/queries/class_field_list.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/template_builtin.hpp>
#include <quxlang/queries/templex_select_template.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include "graph_dump_test_utils.hpp"
#include <quxlang/vmir2/ir2_constexpr_interpreter.hpp>

#include "rpnx/serialization4.hpp"


#include <random>
#include <variant>

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

std::filesystem::path temp_output_file()
{
    auto tempdir = std::filesystem::temp_directory_path();

    auto fname = std::string();
    std::random_device r;
    std::uniform_int_distribution<int> rint(0, 10);
    for (int i = 0; i < 10; i++)
    {
        fname += ('a' + rint(r));
    }
    fname += ".txt";
    return tempdir / fname;
}

namespace
{
    auto test_i32_type() -> quxlang::type_symbol
    {
        return quxlang::int_type{32, true};
    }

    auto test_i32_value(std::byte low_byte) -> quxlang::antestatal_value
    {
        return quxlang::antestatal_primitive{.value = {low_byte, std::byte{0}, std::byte{0}, std::byte{0}}};
    }

    auto expect_i32_value(quxlang::antestatal_value const& value, std::byte low_byte) -> void
    {
        ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(value));
        ASSERT_EQ(quxlang::as< quxlang::antestatal_primitive >(value).value, (std::vector{low_byte, std::byte{0}, std::byte{0}, std::byte{0}}));
    }
} // namespace

static quxlang::parsers::parsing_context test_parsing_context(std::string const& input)
{
    return quxlang::parsers::make_unlocated_parsing_context(input);
}

static quxlang::ast2_file_declaration parse_file_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    return quxlang::parsers::parse_file(ctx);
}

static std::optional< quxlang::ast2_class_declaration > try_parse_class_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    return quxlang::parsers::try_parse_class(ctx);
}

static std::vector< quxlang::ast2_function_parameter > parse_function_args_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    return quxlang::parsers::parse_function_args(ctx);
}

static quxlang::type_symbol parse_type_symbol(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    auto result = quxlang::parsers::parse_type_symbol(ctx);
    if (ctx.iter_pos != ctx.iter_end)
    {
        throw std::logic_error("Input not fully parsed");
    }
    return result;
}

static quxlang::expression parse_expression_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    auto result = quxlang::parsers::parse_expression(ctx);
    if (ctx.iter_pos != ctx.iter_end)
    {
        throw std::logic_error("Input not fully parsed");
    }
    return result;
}

static std::optional< quxlang::expression > try_parse_expression_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    return quxlang::parsers::try_parse_expression(ctx);
}

static quxlang::function_destroy_statement parse_destroy_statement_text(std::string const& input)
{
    auto ctx = test_parsing_context(input);
    auto result = quxlang::parsers::parse_destroy_statement(ctx);
    if (ctx.iter_pos != ctx.iter_end)
    {
        throw std::logic_error("Input not fully parsed");
    }
    return result;
}

TEST(parsing, parse_empty_class)
{
    std::string test_string = "CLASS { }";

    std::optional< quxlang::ast2_class_declaration > cl = try_parse_class_text(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, parse_class_with_variables)
{
    std::string test_string = "CLASS { .a VAR I32; .b VAR I64; ::c VAR I32; }";

    std::optional< quxlang::ast2_class_declaration > cl = try_parse_class_text(test_string);

    ASSERT_TRUE(cl.has_value());

    auto cl2 = cl.value();

    ASSERT_EQ(cl2.declarations.size(), 3);

    auto member1 = cl2.declarations[0];
    auto member2 = cl2.declarations[1];
    auto global = cl2.declarations[2];

    quxlang::subdeclaroid member1_expected = quxlang::member_subdeclaroid{.decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::int_type{32, true})}, .name = "a"};

    quxlang::subdeclaroid member2_expected = quxlang::member_subdeclaroid{.decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::int_type{64, true})}, .name = "b"};

    quxlang::subdeclaroid global_expected = quxlang::global_subdeclaroid{.decl = quxlang::ast2_variable_declaration{quxlang::type_symbol(quxlang::int_type{32, true})}, .name = "c"};

    ASSERT_TRUE(member1 == member1_expected);
    ASSERT_TRUE(member2 == member2_expected);
    ASSERT_TRUE(global == global_expected);
}

TEST(parsing, parse_class_constructor)
{
    std::string test_string = "CLASS { .CONSTRUCTOR FUNCTION(%a I32) { } }";

    auto cl = try_parse_class_text(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, parse_class_constructor_delegates)
{
    std::string test_string = "CLASS { .CONSTRUCTOR FUNCTION(%a I32) :> .x:(1) { } }";

    auto cl = try_parse_class_text(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, parse_class_explicit_constructor_keyword)
{
    std::string test_string = "CLASS { .CONSTRUCTOR FUNCTION(@EXPLICIT I32) { } }";

    auto cl = try_parse_class_text(test_string);

    ASSERT_TRUE(cl.has_value());
}

TEST(parsing, functum_combining)
{
    std::string test_string = "::foo FUNCTION(%a I32) { } ::foo FUNCTION(%a I64) { }";

    quxlang::ast2_file_declaration file;
    file = parse_file_text(test_string);

    ASSERT_EQ(file.declarations.size(), 2);

    // ASSERT_EQ(file.globals[1].first, "foo");
}

TEST(parsing, declaration_doc_block)
{
    std::string test_string = "::foo DOC <$\n    asdf jlk\n    the quick brown fox jumped over the fence\n    $> FUNCTION(%a I32, %b I32) { }";

    auto file = parse_file_text(test_string);

    ASSERT_EQ(file.declarations.size(), 1);
    auto const& global = file.declarations[0].get_as< quxlang::global_subdeclaroid >();
    ASSERT_TRUE(global.doc.has_value());
    EXPECT_EQ(*global.doc, "\nasdf jlk\nthe quick brown fox jumped over the fence\n");
}

TEST(parsing, declaration_doc_block_after_include_if)
{
    std::string test_string = "::foo INCLUDE_IF(ARCH_X64) DOC <$x$> FUNCTION() { }";

    auto file = parse_file_text(test_string);

    ASSERT_EQ(file.declarations.size(), 1);
    auto const& global = file.declarations[0].get_as< quxlang::global_subdeclaroid >();
    ASSERT_TRUE(global.include_if.has_value());
    ASSERT_TRUE(global.doc.has_value());
    EXPECT_EQ(*global.doc, "x");
}

TEST(parsing, declaration_rejects_multiple_doc_blocks)
{
    EXPECT_THROW(parse_file_text("::foo DOC <$x$> DOC <$y$> FUNCTION() { }"), std::logic_error);
}

TEST(parsing, parse_function_args)
{
    std::string test_string = "(%a I32, %b I64, %c -> I32)";

    auto args = parse_function_args_text(test_string);

    ASSERT_EQ(args.size(), 3);
    ASSERT_EQ(args[0].name, "a");
    ASSERT_EQ(args[1].name, "b");
    ASSERT_EQ(args[2].name, "c");

    bool ok1 = args[0].type == quxlang::type_symbol(quxlang::int_type{32, true});
    bool ok2 = args[1].type == quxlang::type_symbol(quxlang::int_type{64, true});
    ASSERT_TRUE(ok1);
    ASSERT_TRUE(ok2);

    auto variadic_args = parse_function_args_text("(%a I32, %...b I32)");
    ASSERT_EQ(variadic_args.size(), 2);
    ASSERT_EQ(variadic_args[0].name, "a");
    ASSERT_FALSE(variadic_args[0].is_pack);
    ASSERT_EQ(variadic_args[1].name, "b");
    ASSERT_TRUE(variadic_args[1].is_pack);
    ASSERT_EQ(variadic_args[1].type, quxlang::type_symbol(quxlang::int_type{32, true}));

    auto ignored_pack_args = parse_function_args_text("(%...IGNORED I32)");
    ASSERT_EQ(ignored_pack_args.size(), 1);
    ASSERT_FALSE(ignored_pack_args[0].name.has_value());
    ASSERT_TRUE(ignored_pack_args[0].is_pack);

    EXPECT_THROW(parse_function_args_text("(%...a I32, %b I32)"), std::logic_error);
    EXPECT_THROW(parse_function_args_text("(%...a I32, %...b I32)"), std::logic_error);
    EXPECT_THROW(parse_function_args_text("(@...a I32)"), std::logic_error);
}

TEST(parsing, parse_basic_types)
{
    using namespace quxlang::parsers;
    using namespace quxlang;

    ASSERT_TRUE(parse_type_symbol("I64") == type_symbol(int_type{64, true}));
    ASSERT_TRUE(parse_type_symbol("-> I64") == type_symbol(ptrref_type{int_type{64, true}}));
    ASSERT_TRUE(parse_type_symbol("STORAGE(I32, I64)") == type_symbol(storage{.storable_types = {int_type{32, true}, int_type{64, true}}}));
    ASSERT_TRUE(parse_type_symbol("STORAGE(I64, I32)") == parse_type_symbol("STORAGE(I32, I64)"));
    ASSERT_TRUE(parse_type_symbol("ALIGNED_STORAGE(4, 8)") == type_symbol(aligned_storage{.size = expression_numeric_literal{"4"}, .align = expression_numeric_literal{"8"}}));
    ASSERT_TRUE(parse_type_symbol("PACK_ARG_TYPE(b, 0)") == type_symbol(pack_arg_type_ref{.pack_name = "b", .index = expression_numeric_literal{"0"}}));

    ASSERT_TRUE(parse_type_symbol("BOOL") == type_symbol(bool_type{}));
}

TEST(parsing, parse_pack_expressions)
{
    using namespace quxlang;

    auto size_expr = parse_expression_text("PACK_SIZE(b)");
    ASSERT_TRUE(size_expr.type_is< expression_pack_size >());
    ASSERT_EQ(size_expr.get_as< expression_pack_size >().pack_name, "b");

    auto arg_expr = parse_expression_text("PACK_ARG(b, 0)");
    ASSERT_TRUE(arg_expr.type_is< expression_pack_arg >());
    ASSERT_EQ(arg_expr.get_as< expression_pack_arg >().pack_name, "b");
    ASSERT_EQ(arg_expr.get_as< expression_pack_arg >().index, expression(expression_numeric_literal{"0"}));
}

TEST(parsing, type_symbol_parenthesized_postfix)
{
    using namespace quxlang::parsers;
    using namespace quxlang;

    auto foo = type_symbol(freebound_identifier{"foo"});
    auto foo_bar = type_symbol(subsymbol{.of = foo, .name = "bar"});
    auto const_foo = type_symbol(ptrref_type{.target = foo, .ptr_class = pointer_class::ref, .qual = qualifier::constant});
    auto const_foo_bar = type_symbol(ptrref_type{.target = foo_bar, .ptr_class = pointer_class::ref, .qual = qualifier::constant});

    ASSERT_EQ(parse_type_symbol("CONST& foo::bar"), const_foo_bar);
    ASSERT_EQ(parse_type_symbol("CONST& (foo::bar)"), const_foo_bar);
    ASSERT_EQ(parse_type_symbol("(CONST& foo)::bar"), type_symbol(subsymbol{.of = const_foo, .name = "bar"}));
    ASSERT_EQ(parse_type_symbol("(CONST& foo)::.bar"), type_symbol(submember{.of = const_foo, .name = "bar"}));

    auto expect_round_trip = [](type_symbol const& symbol, std::string const& expected) {
        auto printed = to_string(symbol);
        ASSERT_EQ(printed, expected);
        ASSERT_EQ(parse_type_symbol(printed), symbol);
    };

    expect_round_trip(const_foo_bar, "CONST& foo::bar");
    expect_round_trip(type_symbol(subsymbol{.of = const_foo, .name = "bar"}), "(CONST& foo)::bar");
    expect_round_trip(type_symbol(submember{.of = const_foo, .name = "bar"}), "(CONST& foo)::.bar");

    ASSERT_TRUE(typeis< instanciation_reference >(parse_type_symbol("foo #{bar}")));
}

TEST(typeutils, named_arguments_print_in_stable_order)
{
    using namespace quxlang;

    auto i32 = type_symbol(int_type{32, true});
    auto i64 = type_symbol(int_type{64, true});
    auto byte = type_symbol(byte_type{});
    auto boolean = type_symbol(bool_type{});
    auto size = type_symbol(size_type{});

    invotype inv{
        .named = {{"ZZZ", boolean}, {"OTHER", i64}, {"THIS", i32}, {"AAA", byte}},
        .positional = {size},
    };
    intertype interface{
        .positional = {argif{.type = size}},
        .named = {
            {"ZZZ", argif{.type = boolean}},
            {"OTHER", argif{.type = i64}},
            {"THIS", argif{.type = i32}},
            {"AAA", argif{.type = byte}},
        },
    };

    ASSERT_EQ(to_string(inv), "INVOTYPE(@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ)");
    ASSERT_EQ(to_string(interface), "INTERTYPE(@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ)");
    ASSERT_EQ(to_string(type_symbol(procedure_type{.signature = sigtype{.params = inv}})), "PROCEDURE(@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ)");

    auto receiver = type_symbol(freebound_identifier{"foo"});
    temploid_reference temploid{.templexoid = receiver, .which = temploid_ensig{.interface = interface}};

    ASSERT_EQ(to_string(type_symbol(initialization_reference{.initializee = receiver, .parameters = quxlang::instatype_from_invotype(inv)})), "foo #(@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ)");
    ASSERT_EQ(to_string(type_symbol(temploid)), "foo#[@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ]");
    ASSERT_EQ(to_string(type_symbol(instanciation_reference{.temploid = temploid, .params = quxlang::instatype_from_invotype(inv)})), "foo#{@THIS I32, @OTHER I64, @AAA BYTE, @ZZZ BOOL, SZ}");
}

TEST(typeutils, value_template_arguments_print_with_equals)
{
    using namespace quxlang;

    auto i32 = type_symbol(int_type{32, true});
    auto receiver = type_symbol(freebound_identifier{"foo"});
    auto value = constexpr_value(test_i32_value(std::byte{4}));
    temploid_reference named_temploid{
        .templexoid = receiver,
        .which = temploid_ensig{.interface = intertype{.named = {{"foo", argif{.type = i32, .requires_static_value = true}}}}},
    };
    instatype named_params{.named = {{"foo", parameter_value_instantiation{.type = i32, .value = value}}}};
    ASSERT_EQ(to_string(type_symbol(instanciation_reference{.temploid = named_temploid, .params = named_params})), "foo#{@foo I32=4}");

    temploid_reference positional_temploid{
        .templexoid = receiver,
        .which = temploid_ensig{.interface = intertype{.positional = {argif{.type = i32, .requires_static_value = true}}}},
    };
    instatype positional_params{.positional = {parameter_value_instantiation{.type = i32, .value = value}}};
    ASSERT_EQ(to_string(type_symbol(instanciation_reference{.temploid = positional_temploid, .params = positional_params})), "foo#{I32=4}");
}

TEST(parsing, parse_new_template_parameter_kinds)
{
    auto file = parse_file_text("::foo TEMPLATE(@T TYPE, @U TYPE AUTO(u), @N VALUE I32) CLASS {}");
    ASSERT_EQ(file.declarations.size(), 1);
    auto const& decl = quxlang::as< quxlang::global_subdeclaroid >(file.declarations.front());
    auto const& tmpl = quxlang::as< quxlang::ast2_template_declaration >(decl.decl);

    ASSERT_EQ(tmpl.m_template_args.named.at("T").kind, quxlang::template_parameter_kind::type);
    ASSERT_EQ(tmpl.m_template_args.named.at("T").type, quxlang::type_symbol(quxlang::type_temploidic{"T"}));
    ASSERT_EQ(tmpl.m_template_args.named.at("U").kind, quxlang::template_parameter_kind::type);
    ASSERT_EQ(tmpl.m_template_args.named.at("U").type, parse_type_symbol("AUTO(u)"));
    ASSERT_EQ(tmpl.m_template_args.named.at("N").kind, quxlang::template_parameter_kind::value);
    ASSERT_EQ(tmpl.m_template_args.named.at("N").type, parse_type_symbol("I32"));

    auto local_file = parse_file_text("::local_name TEMPLATE(@api:local VALUE I32) CLASS {}");
    auto const& local_decl = quxlang::as< quxlang::global_subdeclaroid >(local_file.declarations.front());
    auto const& local_tmpl = quxlang::as< quxlang::ast2_template_declaration >(local_decl.decl);
    ASSERT_EQ(local_tmpl.m_template_args.named.at("api").name, std::optional< std::string >{"local"});
    ASSERT_EQ(local_tmpl.m_template_args.named.at("api").kind, quxlang::template_parameter_kind::value);

    EXPECT_NO_THROW(parse_file_text("::bar TEMPLATE(TYPE) CLASS {}"));
    EXPECT_THROW(parse_file_text("::old TEMPLATE(@T AUTO) CLASS {}"), std::logic_error);
}

TEST(parsing, initialization_reference_arguments_are_expressions)
{
    using namespace quxlang;

    auto positional = parse_type_symbol("foo#(I32)");
    ASSERT_TRUE(typeis< initialization_reference >(positional));
    auto const& positional_init = as< initialization_reference >(positional);
    ASSERT_EQ(positional_init.arguments.size(), 1);
    ASSERT_EQ(positional_init.parameters.size(), 0);
    ASSERT_TRUE(typeis< expression_symbol_reference >(positional_init.arguments.front().value));

    auto named = parse_type_symbol("foo#(@N 1)");
    ASSERT_TRUE(typeis< initialization_reference >(named));
    auto const& named_init = as< initialization_reference >(named);
    ASSERT_EQ(named_init.arguments.size(), 1);
    ASSERT_EQ(named_init.arguments.front().name, std::optional< std::string >{"N"});
    ASSERT_TRUE(typeis< expression_numeric_literal >(named_init.arguments.front().value));
}

TEST(parsing, parse_global_constexpr_variable_declaration)
{
    std::string test_string = "::foobar VAR CONSTEXPR_READABLE I32 := 4;";

    quxlang::ast2_file_declaration file = parse_file_text(test_string);

    ASSERT_EQ(file.declarations.size(), 1);
    ASSERT_TRUE(quxlang::typeis< quxlang::global_subdeclaroid >(file.declarations.front()));

    auto const& decl = quxlang::as< quxlang::global_subdeclaroid >(file.declarations.front());
    ASSERT_EQ(decl.name, "foobar");
    ASSERT_TRUE(quxlang::typeis< quxlang::ast2_variable_declaration >(decl.decl));

    auto const& variable_decl = quxlang::as< quxlang::ast2_variable_declaration >(decl.decl);
    ASSERT_EQ(variable_decl.type, quxlang::type_symbol(quxlang::int_type{32, true}));
    ASSERT_TRUE(variable_decl.keyword_tags.contains("CONSTEXPR_READABLE"));
    ASSERT_FALSE(variable_decl.keyword_tags.contains("CONSTEXPR_READWRITE"));
    ASSERT_TRUE(variable_decl.init_expr.has_value());
    ASSERT_EQ(quxlang::to_string(*variable_decl.init_expr), "4");
    ASSERT_TRUE(variable_decl.init_args.empty());
}

TEST(parsing, parse_global_static_variable_declaration)
{
    std::string test_string = "::foo STATIC I32 := 4;";

    quxlang::ast2_file_declaration file = parse_file_text(test_string);

    ASSERT_EQ(file.declarations.size(), 1);
    ASSERT_TRUE(quxlang::typeis< quxlang::global_subdeclaroid >(file.declarations.front()));

    auto const& decl = quxlang::as< quxlang::global_subdeclaroid >(file.declarations.front());
    ASSERT_EQ(decl.name, "foo");
    ASSERT_TRUE(quxlang::typeis< quxlang::ast2_variable_declaration >(decl.decl));

    auto const& variable_decl = quxlang::as< quxlang::ast2_variable_declaration >(decl.decl);
    ASSERT_EQ(variable_decl.type, quxlang::type_symbol(quxlang::int_type{32, true}));
    ASSERT_TRUE(variable_decl.keyword_tags.contains("STATIC"));
    ASSERT_TRUE(variable_decl.init_expr.has_value());
    ASSERT_EQ(quxlang::to_string(*variable_decl.init_expr), "4");
    ASSERT_TRUE(variable_decl.init_args.empty());
}

TEST(parsing, parse_function_local_static_statements)
{
    std::string test_string = R"QX(
::foo STATIC_TEST
{
  STATIC a I32;
  STATIC b I32 :(@OTHER 7);
  STATIC c I32 := 8;
  STATIC_VAR d I32;
  STATIC_VAR e I32 :(@OTHER 9);
  STATIC_VAR f I32 := 10;
  STATIC_EVAL d++;
  VAR g I32 := SNAPSHOT(d);
  STATIC_IF (TRUE) {
  } STATIC_ELSE STATIC_IF (FALSE) {
  } STATIC_ELSE {
  }
  STATIC_WHILE (FALSE) {
  }
}
)QX";

    quxlang::ast2_file_declaration file = parse_file_text(test_string);
    ASSERT_EQ(file.declarations.size(), 1);
    auto const& decl = quxlang::as< quxlang::global_subdeclaroid >(file.declarations.front());
    auto const& test = quxlang::as< quxlang::ast2_static_test >(decl.decl);
    auto const& statements = test.definition.body.statements;

    ASSERT_GE(statements.size(), 10);
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(0)).static_kind, std::optional{quxlang::function_static_kind::constant});
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(1)).static_kind, std::optional{quxlang::function_static_kind::constant});
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(2)).static_kind, std::optional{quxlang::function_static_kind::constant});
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(3)).static_kind, std::optional{quxlang::function_static_kind::mutable_});
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(4)).static_kind, std::optional{quxlang::function_static_kind::mutable_});
    ASSERT_EQ(quxlang::as< quxlang::function_var_statement >(statements.at(5)).static_kind, std::optional{quxlang::function_static_kind::mutable_});
    ASSERT_TRUE(quxlang::typeis< quxlang::function_static_eval_statement >(statements.at(6)));
    ASSERT_TRUE(quxlang::typeis< quxlang::function_var_statement >(statements.at(7)));
    ASSERT_TRUE(quxlang::typeis< quxlang::expression_snapshot >(*quxlang::as< quxlang::function_var_statement >(statements.at(7)).equals_initializer));
    ASSERT_TRUE(quxlang::typeis< quxlang::function_static_if_statement >(statements.at(8)));
    ASSERT_TRUE(quxlang::typeis< quxlang::function_static_while_statement >(statements.at(9)));
}

TEST(parsing, reject_static_else_mismatch)
{
    EXPECT_THROW(parse_file_text("::foo STATIC_TEST { STATIC_IF (TRUE) { } ELSE { } }"), std::logic_error);
    EXPECT_THROW(parse_file_text("::foo STATIC_TEST { IF (TRUE) { } STATIC_ELSE { } }"), std::logic_error);
}

TEST(parsing, reject_global_and_class_static_var)
{
    EXPECT_THROW(parse_file_text("::foo STATIC_VAR I32;"), std::logic_error);
    EXPECT_THROW(parse_file_text("::foo CLASS { .bar STATIC_VAR I32; }"), std::logic_error);
}

TEST(mangling, name_mangling_new)
{
    quxlang::absolute_module_reference module{"main"};

    quxlang::subsymbol subentity{module, "foo"};

    quxlang::subsymbol subentity2;
    subentity2.of = subentity;
    subentity2.name = "bar";

    quxlang::subsymbol subentity3;
    subentity3.of = subentity2;
    subentity3.name = "baz";

    quxlang::initialization_reference param_set{subentity3, {}};

    param_set.parameters.positional.push_back(quxlang::make_type_instantiation(quxlang::int_type{32, true}));
    param_set.parameters.positional.push_back(quxlang::make_type_instantiation(quxlang::int_type{32, true}));

    std::string mangled_name = quxlang::mangle(quxlang::type_symbol(param_set));

    ASSERT_EQ(mangled_name, "_S_MmainNfooNbarNbazCAPI32API32E");
}

TEST(collector_tester, order_of_operations)
{
    // quxlang::collector c;

    std::string test_string = "a + b * c + d + e * f := g + h ^^ i * j * k * l := m + n && o + p";

    quxlang::expression expr;

    auto ctx = test_parsing_context(test_string);
    expr = quxlang::parsers::parse_expression(ctx);

    std::string str = quxlang::to_string(expr);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_binary >());
    ASSERT_TRUE(as< quxlang::expression_binary >(expr).operator_str == ":=");
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
};


TEST(collector_tester, function_call)
{

    std::string test_string = "e.a.b(c, d, e.f)";

    quxlang::expression expr;

    auto ctx = test_parsing_context(test_string);
    expr = quxlang::parsers::parse_expression(ctx);

    std::string str = quxlang::to_string(expr);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_call >());
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
};

TEST(parsing, constexpr_allocator_call_expression)
{
    auto expr = parse_expression_text("CONSTEXPR_ALLOC#(I32)()");
    ASSERT_TRUE(expr.template type_is< quxlang::expression_call >());

    auto const& call = expr.get_as< quxlang::expression_call >();
    ASSERT_TRUE(call.callee.template type_is< quxlang::expression_symbol_reference >());

    auto const& callee = call.callee.get_as< quxlang::expression_symbol_reference >();
    ASSERT_TRUE(callee.symbol.template type_is< quxlang::initialization_reference >());

    auto const& init = callee.symbol.get_as< quxlang::initialization_reference >();
    ASSERT_TRUE(init.initializee.template type_is< quxlang::freebound_identifier >());
    EXPECT_EQ(init.initializee.get_as< quxlang::freebound_identifier >().name, "CONSTEXPR_ALLOC");
    ASSERT_EQ(init.arguments.size(), 1);
    EXPECT_TRUE(call.args.empty());
}

TEST(parsing, constexpr_allocator_storage_statements)
{
    EXPECT_NO_THROW(parse_file_text(R"QX(
::constexpr_allocator_parse_smoke FUNCTION(): VOID
{
  VAR count SZ := 2;
  VAR slots =>> STORAGE(BYTE) := CONSTEXPR_ALLOC_MULTIPLE#(BYTE)(count);
  PLACE AT((slots + 1)->) BYTE := 9;
  ASSERT((PUN ((slots + 1)->) AS BYTE) == 9);
  DESTROY AT((slots + 1)->) BYTE;
  CONSTEXPR_DEALLOC_MULTIPLE#(BYTE)(slots, count);
}
)QX"));
}

TEST(parsing, parse_bitwise_inverse_postfix)
{
    std::string test_string = "bu #!!";

    auto ctx = test_parsing_context(test_string);
    quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_unary_postfix >());
    ASSERT_EQ(quxlang::as< quxlang::expression_unary_postfix >(expr).operator_str, "#!!");
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
}

TEST(parsing, parse_logical_inverse_postfix)
{
    std::string test_string = "bu !!";

    auto ctx = test_parsing_context(test_string);
    quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_unary_postfix >());
    ASSERT_EQ(quxlang::as< quxlang::expression_unary_postfix >(expr).operator_str, "!!");
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
}

TEST(parsing, parse_extended_logical_operators)
{
    for (std::string const& test_string : {"a &! b", "a |! b", "a ^! b", "a ^> b", "a ^< b"})
    {
        auto ctx = test_parsing_context(test_string);
        quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

        ASSERT_TRUE(expr.template type_is< quxlang::expression_binary >());
        ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
    }
}

TEST(source_locations, source_location_string_format)
{
    ASSERT_EQ(quxlang::to_string(quxlang::source_location{.file_id = 0, .begin_index = 2, .end_index = std::optional< std::size_t >{6}}), "@@(0, 2, 6)");
    ASSERT_EQ(quxlang::to_string(quxlang::source_location{.file_id = 0, .begin_index = 2}), "@@(0, 2)");
}

TEST(source_locations, expression_locations_are_hidden_unless_requested)
{
    quxlang::expression expr = quxlang::expression_numeric_literal{
        .value = "4",
        .location = quxlang::source_location{.file_id = 0, .begin_index = 2, .end_index = std::optional< std::size_t >{6}},
    };

    ASSERT_EQ(quxlang::to_string(expr), "4");
    ASSERT_EQ(quxlang::to_string(expr, true), "4 @@(0, 2, 6)");
}

TEST(source_locations, context_expression_parser_assigns_recursive_locations)
{
    std::string source = "STATIC_CHOOSE(1, 2, 3)";
    quxlang::parsers::parsing_context ctx{
        .file_id = 17,
        .source_locations_enabled = true,
        .iter_begin = source.begin(),
        .iter_pos = source.begin(),
        .iter_end = source.end(),
    };

    quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

    ASSERT_EQ(quxlang::to_string(expr, true), "STATIC_CHOOSE( 1 @@(17, 14, 15) , 2 @@(17, 17, 18) , 3 @@(17, 20, 21) ) @@(17, 0, 22)");
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
}

TEST(source_locations, vmir_instruction_and_terminator_locations_print_automatically)
{
    quxlang::vmir2::functanoid_routine3 routine;
    quxlang::vmir2::assembler assembler(routine);
    quxlang::source_location loc{.file_id = 123, .begin_index = 2, .end_index = std::optional< std::size_t >{6}};

    quxlang::vmir2::vm_instruction located_instruction = quxlang::vmir2::assert_instr{
        .condition = quxlang::vmir2::local_index(1),
        .message = "ok",
        .location = loc,
    };
    quxlang::vmir2::vm_instruction unlocated_instruction = quxlang::vmir2::assert_instr{
        .condition = quxlang::vmir2::local_index(1),
        .message = "ok",
    };
    quxlang::vmir2::vm_terminator located_terminator = quxlang::vmir2::ret{.location = loc};

    ASSERT_EQ(assembler.to_string(located_instruction), "ASSERT %1, \"ok\" @@(123, 2, 6)");
    ASSERT_EQ(assembler.to_string(unlocated_instruction), "ASSERT %1, \"ok\"");
    ASSERT_EQ(assembler.to_string(located_terminator), "RET @@(123, 2, 6)");

    quxlang::source_file_name file_name{.source_module = "foo", .relative_path = "bar.qx"};
    quxlang::source_file_index file_index;
    file_index.file_to_id.emplace(file_name, 123);
    file_index.id_to_file.emplace(123, file_name);

    quxlang::source_bundle bundle;
    bundle.module_sources["foo"].files["bar.qx"] = quxlang::source_file{.contents = "a\nbcdefgh\n"};

    quxlang::vmir2::assembler source_assembler(routine, quxlang::vmir2::source_index(file_index, bundle));
    ASSERT_EQ(source_assembler.to_string(located_instruction), "ASSERT %1, \"ok\" @@ foo/bar.qx:2:1,2:5");
    ASSERT_EQ(source_assembler.to_string(located_terminator), "RET @@ foo/bar.qx:2:1,2:5");

    quxlang::source_file_name loaded_file_name{.source_module = "main", .relative_path = "modules/main/sources/bar.qx"};
    quxlang::source_file_index loaded_file_index;
    loaded_file_index.file_to_id.emplace(loaded_file_name, 123);
    loaded_file_index.id_to_file.emplace(123, loaded_file_name);

    quxlang::source_bundle loaded_bundle;
    loaded_bundle.module_sources["main"].files["modules/main/sources/bar.qx"] = quxlang::source_file{.contents = "a\nbcdefgh\n"};

    quxlang::vmir2::assembler loaded_source_assembler(routine, quxlang::vmir2::source_index(loaded_file_index, loaded_bundle));
    ASSERT_EQ(loaded_source_assembler.to_string(located_instruction), "ASSERT %1, \"ok\" @@ modules/main/sources/bar.qx:2:1,2:5");

    routine.local_types.push_back(quxlang::vmir2::local_type{.type = quxlang::int_type{32, true}});
    routine.local_types.push_back(quxlang::vmir2::local_type{.type = quxlang::int_type{32, true}});
    quxlang::vmir2::assembler typed_comment_assembler(routine);
    quxlang::vmir2::vm_instruction located_typed_comment_instruction = quxlang::vmir2::access_field{
        .base_index = quxlang::vmir2::local_index(0),
        .store_index = quxlang::vmir2::local_index(1),
        .field_name = "x",
        .location = loc,
    };
    ASSERT_EQ(typed_comment_assembler.to_string(located_typed_comment_instruction), "ACCESS_FIELD %0, %1, x @@(123, 2, 6) // type1=I32 type2=I32");

    quxlang::vmir2::assembler typed_comment_source_assembler(routine, quxlang::vmir2::source_index(file_index, bundle));
    ASSERT_EQ(typed_comment_source_assembler.to_string(located_typed_comment_instruction), "ACCESS_FIELD %0, %1, x @@ foo/bar.qx:2:1,2:5 // type1=I32 type2=I32");
}

TEST(vmir_constexpr_interpreter, localdata_get_antestatal_ref_sets_result_zero)
{
    auto symbol = quxlang::type_symbol(quxlang::static_local_ref{.functanoid = quxlang::void_type{}, .name = "x", .generation = 1});
    auto i32 = test_i32_type();

    quxlang::vmir2::functanoid_routine3 routine;
    routine.local_types = {
        quxlang::vmir2::local_type{.type = quxlang::void_type{}},
        quxlang::vmir2::local_type{.type = quxlang::make_cref(i32)},
    };
    routine.blocks.push_back(quxlang::vmir2::executable_block{
        .instructions = {
            quxlang::vmir2::get_antestatal_ref{.symbol = symbol, .target_ref = quxlang::vmir2::local_index(1)},
            quxlang::vmir2::constexpr_set_result2{.target = quxlang::vmir2::local_index(1), .target_mode = quxlang::vmir2::constexpr_result_target_mode::referenced_object},
        },
        .terminator = quxlang::vmir2::ret{},
    });

    quxlang::vmir2::ir2_constexpr_interpreter interp;
    interp.add_constexpr_antestatal_global(symbol, i32, test_i32_value(std::byte{7}), false);
    interp.add_functanoid3(quxlang::void_type{}, routine);
    interp.exec3(quxlang::void_type{});

    expect_i32_value(interp.get_cr_antestatal_value(), std::byte{7});
}

TEST(vmir_constexpr_interpreter, localdata_mutability_controls_store_to_ref)
{
    auto symbol = quxlang::type_symbol(quxlang::static_local_ref{.functanoid = quxlang::void_type{}, .name = "x", .generation = 2});
    auto i32 = test_i32_type();

    auto make_routine = [&]()
    {
        quxlang::vmir2::functanoid_routine3 routine;
        routine.local_types = {
            quxlang::vmir2::local_type{.type = quxlang::void_type{}},
            quxlang::vmir2::local_type{.type = quxlang::make_mref(i32)},
            quxlang::vmir2::local_type{.type = i32},
            quxlang::vmir2::local_type{.type = quxlang::make_mref(i32)},
        };
        routine.blocks.push_back(quxlang::vmir2::executable_block{
            .instructions = {
                quxlang::vmir2::get_antestatal_ref{.symbol = symbol, .target_ref = quxlang::vmir2::local_index(1)},
                quxlang::vmir2::load_const_int{.target = quxlang::vmir2::local_index(2), .value = "9"},
                quxlang::vmir2::store_to_ref{.from_value = quxlang::vmir2::local_index(2), .to_reference = quxlang::vmir2::local_index(1)},
                quxlang::vmir2::get_antestatal_ref{.symbol = symbol, .target_ref = quxlang::vmir2::local_index(3)},
                quxlang::vmir2::constexpr_set_result2{
                    .target = quxlang::vmir2::local_index(3),
                    .result_id = 17,
                    .target_mode = quxlang::vmir2::constexpr_result_target_mode::referenced_object,
                },
            },
            .terminator = quxlang::vmir2::ret{},
        });
        return routine;
    };

    quxlang::vmir2::ir2_constexpr_interpreter immutable_interp;
    immutable_interp.add_constexpr_antestatal_global(symbol, i32, test_i32_value(std::byte{3}), false);
    immutable_interp.add_functanoid3(quxlang::void_type{}, make_routine());
    EXPECT_THROW(immutable_interp.exec3(quxlang::void_type{}), quxlang::constexpr_logic_execution_error);

    quxlang::vmir2::ir2_constexpr_interpreter mutable_interp;
    mutable_interp.add_constexpr_antestatal_global(symbol, i32, test_i32_value(std::byte{3}), true);
    mutable_interp.add_functanoid3(quxlang::void_type{}, make_routine());
    mutable_interp.exec3(quxlang::void_type{});

    auto result_values = mutable_interp.get_cr_antestatal_values();
    ASSERT_TRUE(result_values.contains(17));
    expect_i32_value(result_values.at(17), std::byte{9});
}

TEST(vmir_constexpr_interpreter, missing_static_localdata_is_compiler_bug)
{
    auto symbol = quxlang::type_symbol(quxlang::static_snapshot_ref{.functanoid = quxlang::void_type{}, .name = "x", .generation = 1, .snapshot_id = 3});
    auto i32 = test_i32_type();

    quxlang::vmir2::functanoid_routine3 routine;
    routine.local_types = {
        quxlang::vmir2::local_type{.type = quxlang::void_type{}},
        quxlang::vmir2::local_type{.type = quxlang::make_cref(i32)},
    };
    routine.blocks.push_back(quxlang::vmir2::executable_block{
        .instructions = {
            quxlang::vmir2::get_antestatal_ref{.symbol = symbol, .target_ref = quxlang::vmir2::local_index(1)},
        },
        .terminator = quxlang::vmir2::ret{},
    });

    quxlang::vmir2::ir2_constexpr_interpreter interp;
    EXPECT_THROW(interp.add_functanoid3(quxlang::void_type{}, routine), quxlang::compiler_bug);
}

TEST(parsing, parse_pun_expression)
{
    std::string test_string = "PUN x AS I32";

    auto ctx = test_parsing_context(test_string);
    quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_pun >());
    ASSERT_EQ(quxlang::to_string(quxlang::as< quxlang::expression_pun >(expr).as_type), "I32");
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
}

TEST(parsing, parse_place_expression)
{
    std::string test_string = "PLACE AT(x) I32:(y)";

    auto ctx = test_parsing_context(test_string);
    quxlang::expression expr = quxlang::parsers::parse_expression(ctx);

    ASSERT_TRUE(expr.template type_is< quxlang::expression_place >());
    ASSERT_EQ(quxlang::to_string(quxlang::as< quxlang::expression_place >(expr).type), "I32");
    ASSERT_EQ(quxlang::as< quxlang::expression_place >(expr).args.size(), 1);
    ASSERT_EQ(ctx.iter_pos, ctx.iter_end);
}

TEST(parsing, parse_partial_cast_expression)
{
    std::string test_string = "x AS PARTIAL I16";

    auto expr = try_parse_expression_text(test_string);

    ASSERT_TRUE(expr.has_value());
    ASSERT_TRUE(expr->template type_is< quxlang::expression_typecast >());
    ASSERT_EQ(expr->template get_as< quxlang::expression_typecast >().keyword.value(), "PARTIAL");
}

TEST(parsing, reject_internal_tempar_name_in_expression)
{
    EXPECT_ANY_THROW(parse_expression_text("IS_INTEGRAL(__int_type)"));
}



TEST(parsing, parse_destroy_statement_with_args)
{
    std::string test_string = "DESTROY AT(x) I32:(y);";

    auto st = parse_destroy_statement_text(test_string);

    ASSERT_EQ(quxlang::to_string(st.type), "I32");
    ASSERT_EQ(st.args.size(), 1);
}

TEST(quxlang_modules, merge_entities)
{
    // TODO: Needs rewrite with the replaced merger
}

TEST(qual, template_matching)
{
    quxlang::type_symbol template1 = quxlang::auto_temploidic{"foo"};
    quxlang::type_symbol template2 = quxlang::ptrref_type{.target = quxlang::auto_temploidic{"foo"}, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut};
    quxlang::type_symbol type1 = quxlang::int_type{32, true};
    quxlang::type_symbol type2 = quxlang::ptrref_type{.target = quxlang::int_type{32, true}, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut};

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

TEST(qual, template_matching_preserves_reference_shape_without_hidden_conversions)
{
    auto templ = parse_type_symbol("CONST& AUTO(t)");
    auto source = parse_type_symbol("MUT& I32");

    auto match = quxlang::match_template(templ, source);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->type, parse_type_symbol("CONST& I32"));
    EXPECT_EQ(match->matches.at("t"), parse_type_symbol("I32"));

    auto auto_ref = parse_type_symbol("& AUTO(t)");
    auto value = parse_type_symbol("I32");
    EXPECT_FALSE(quxlang::match_template(auto_ref, value).has_value());
}



#include "expr_test_provider.hpp"
#include "quxlang/source_loader.hpp"
#include <gtest/gtest.h>
#include <list>
#include <string>
#include <vector>

TEST(SerializationTest, IntegralTypes)
{
    std::vector< std::byte > buffer;

    // Serialize integral types
    std::uint32_t uint32_value = 0x12345678;
    std::int64_t int64_value = -0x1234567890ABCDEF;

    rpnx::serial4::serialize_iter(uint32_value, std::back_inserter(buffer));
    rpnx::serial4::serialize_iter(int64_value, std::back_inserter(buffer));

    // Deserialize integral types
    auto it = buffer.begin();
    std::uint32_t deserialized_uint32;
    std::int64_t deserialized_int64;

    it = rpnx::serial4::deserialize_iter(deserialized_uint32, it);
    it = rpnx::serial4::deserialize_iter(deserialized_int64, it);

    EXPECT_EQ(uint32_value, deserialized_uint32);
    EXPECT_EQ(int64_value, deserialized_int64);
}

TEST(SerializationTest, Map)
{
    std::vector< std::byte > buffer;

    // Serialize map
    std::map< std::uint32_t, std::string > map = {{1, "one"}, {2, "two"}, {3, "three"}};

    rpnx::serial4::serialize_iter(map, std::back_inserter(buffer));

    // Deserialize map
    std::map< std::uint32_t, std::string > deserialized_map;
    auto it = buffer.begin();

    it = rpnx::serial4::deserialize_iter(deserialized_map, it);

    EXPECT_EQ(map, deserialized_map);
}

TEST(SerializationTest, Vector)
{
    std::vector< std::byte > buffer;

    // Serialize vector
    std::vector< std::uint32_t > vector = {1, 2, 3, 4, 5};

    rpnx::serial4::serialize_iter(vector, std::back_inserter(buffer));

    // Deserialize vector
    std::vector< std::uint32_t > deserialized_vector;
    auto it = buffer.begin();

    it = rpnx::serial4::deserialize_iter(deserialized_vector, it);

    EXPECT_EQ(vector, deserialized_vector);
}

TEST(SerializationTest, Set)
{
    std::vector< std::byte > buffer;

    // Serialize set
    std::set< std::string > set = {"one", "two", "three"};

    rpnx::serial4::serialize_iter(set, std::back_inserter(buffer));

    // Deserialize set
    std::set< std::string > deserialized_set;
    auto it = buffer.begin();

    it = rpnx::serial4::deserialize_iter(deserialized_set, it);

    EXPECT_EQ(set, deserialized_set);
}

TEST(SerializerTest, TupleSerializationDeserialization)
{
    std::tuple< int, std::string, int > original{42, "hello", 3};

    // Serialization
    std::vector< std::byte > bytes;
    rpnx::serial4::serialize_iter(original, std::back_inserter(bytes));

    // Deserialization
    std::tuple< int, std::string, int > deserialized;
    auto it = rpnx::serial4::deserialize_iter(deserialized, bytes.begin(), bytes.end());
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
    rpnx::serial4::serialize_iter(std::tie(a, b, c), std::back_inserter(bytes));

    // Deserialization
    int a_deserialized;
    std::string b_deserialized;
    int c_deserialized;
    auto it = rpnx::serial4::deserialize_iter(std::tie(a_deserialized, b_deserialized, c_deserialized), bytes.begin(), bytes.end());
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
    rpnx::serial4::serialize_iter(original, std::back_inserter(bytes));

    // Deserialization
    std::tuple<> deserialized;
    auto it = rpnx::serial4::deserialize_iter(deserialized, bytes.begin(), bytes.end());
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
    rpnx::serial4::serialize_iter(v1, std::back_inserter(buffer));
    rpnx::serial4::serialize_iter(v2, std::back_inserter(buffer2));

    rpnx::variant< int, std::string > v3;
    rpnx::variant< int, std::string > v4;
    rpnx::serial4::deserialize_iter(v3, buffer.cbegin(), buffer.cend());
    rpnx::serial4::deserialize_iter(v4, buffer2.cbegin(), buffer2.cend());

    EXPECT_EQ(v1, v3);
    EXPECT_EQ(v2, v4);

    EXPECT_EQ(v4, std::string("hello"));
}

namespace
{
    class test_querygraph_compiler
    {
      public:
        test_querygraph_compiler(quxlang::source_bundle const& sources, std::string target)
            : m_graph(sources, target, sources.targets.at(target).target_output_config, quxlang::tests::current_test_graph_dump_path())
        {
        }

        auto get_ensig_argument_initialize(quxlang::argument_init_input input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::ensig_argument_initialize_query >(
                quxlang::argument_init_input{.from = std::move(input.from), .to = std::move(input.to), .adaptations = input.adaptations});
        }

        auto get_convertible_by_call(quxlang::implicitly_convertible_to_input input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::convertible_by_call_query >(
                quxlang::implicitly_convertible_to_input{.from = std::move(input.from), .to = std::move(input.to)});
        }

        auto get_constexpr_bool(quxlang::constexpr_input input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::constexpr_bool_query >(std::move(input));
        }

        auto get_instanciation(quxlang::initialization_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::instanciation_query >(std::move(input));
        }

        auto get_functum_user_overloads(quxlang::type_symbol input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::functum_user_overloads_query >(std::move(input));
        }

        auto get_function_builtin(quxlang::temploid_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::function_builtin_query >(std::move(input));
        }

        auto get_vm_procedure3(quxlang::instanciation_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::vm_procedure3_query >(std::move(input));
        }

        auto get_functum_builtin_overloads(quxlang::type_symbol input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::functum_builtin_overloads_query >(std::move(input));
        }

        auto get_lookup(quxlang::contextual_type_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::lookup_query >(std::move(input));
        }

        auto get_templex_select_template(quxlang::initialization_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::templex_select_template_query >(std::move(input));
        }

        auto get_template_builtin(quxlang::temploid_reference input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::template_builtin_query >(std::move(input));
        }

        auto get_symbol_type(quxlang::type_symbol input, std::optional< std::filesystem::path > const&) const
        {
            return m_graph.make_request< quxlang::symbol_type_query >(std::move(input));
        }

      private:
        mutable quxlang::compiler_querygraph m_graph;
    };

    quxlang::source_bundle make_main_module_source_bundle(std::string source)
    {
        quxlang::source_bundle sources;

        quxlang::target_configuration target;
        target.target_output_config.cpu_type = quxlang::cpu::x86_64;
        target.target_output_config.os_type = quxlang::os::linux;
        target.target_output_config.binary_type = quxlang::binary::elf;
        target.module_configurations["main"] = quxlang::module_configuration{.source = "main"};
        sources.targets["linux-x64"] = target;

        quxlang::module_source main_module;
        main_module.files["main.qx"] = quxlang::source_file{.contents = std::move(source)};
        sources.module_sources["main"] = std::move(main_module);

        return sources;
    }

    test_querygraph_compiler make_main_module_compiler(std::string source)
    {
        quxlang::source_bundle sources = make_main_module_source_bundle(std::move(source));
        return test_querygraph_compiler(sources, "linux-x64");
    }

} // namespace

TEST(quxlang, compiler_graph_resolves_main_source_bundle)
{
    quxlang::source_bundle sources = make_main_module_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph(sources, "linux-x64", sources.targets.at("linux-x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto machine_info = graph.make_request< quxlang::machine_info_query >(std::monostate{});
    ASSERT_EQ(machine_info.cpu_type, quxlang::cpu::x86_64);
    ASSERT_EQ(machine_info.os_type, quxlang::os::linux);
    ASSERT_EQ(machine_info.binary_type, quxlang::binary::elf);

    ASSERT_EQ(graph.make_request< quxlang::module_source_name_query >("main"), "main");

    auto module = graph.make_request< quxlang::module_sources_query >("main");
    ASSERT_TRUE(module.files.contains("main.qx"));
    ASSERT_EQ(module.files.at("main.qx").get().contents, "::main VAR I32;");
}

TEST(quxlang, value_template_parameter_is_available_in_class_field_type)
{
    quxlang::source_bundle sources = make_main_module_source_bundle("::holder TEMPLATE(@count:value_count VALUE U64) CLASS { .values VAR [value_count]U64; }");
    quxlang::compiler_querygraph graph(sources, "linux-x64", sources.targets.at("linux-x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto holder = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = parse_type_symbol("MODULE(main)"),
        .type = parse_type_symbol("MODULE(main)::holder#(@count 4)"),
    });
    ASSERT_TRUE(holder.has_value());

    auto fields = graph.make_request< quxlang::class_field_list_query >(*holder);
    ASSERT_EQ(fields.size(), 1);
    ASSERT_EQ(fields.front().name, "values");
    ASSERT_TRUE(quxlang::typeis< quxlang::array_type >(fields.front().type));

    auto const& array = quxlang::as< quxlang::array_type >(fields.front().type);
    ASSERT_EQ(array.element_type, parse_type_symbol("U64"));
    ASSERT_EQ(array.element_count, quxlang::expression(quxlang::expression_numeric_literal{.value = "4"}));
}

TEST(quxlang, ensig_argument_initialize_materializes_value_for_template_reference)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    test_querygraph_compiler c(sources, "linux-x64");

    auto adapted = c.get_ensig_argument_initialize(quxlang::argument_init_input{
                                                       .from = parse_type_symbol("I32"),
                                                       .to = parse_type_symbol("CONST& AUTO(t)"),
                                                       .adaptations = quxlang::allowed_adaptations::destination_rebinding,
                                                   },
                                                   std::nullopt);

    ASSERT_TRUE(adapted.has_value());
    EXPECT_EQ(*adapted, parse_type_symbol("CONST& I32"));
}

TEST(quxlang, write_reference_does_not_objectize_into_auto_template)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    test_querygraph_compiler c(sources, "linux-x64");

    auto adapted = c.get_ensig_argument_initialize(quxlang::argument_init_input{
                                                       .from = parse_type_symbol("WRITE& I32"),
                                                       .to = parse_type_symbol("AUTO(t)"),
                                                       .adaptations = quxlang::allowed_adaptations::destination_rebinding,
                                                   },
                                                   std::nullopt);

    EXPECT_FALSE(adapted.has_value());
}

TEST(quxlang, templating_typoids_consider_ref_rebinding_and_objectization_once)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    test_querygraph_compiler c(sources, "linux-x64");

    auto ref_source = parse_type_symbol("MUT& I32");

    auto tt_const = c.get_ensig_argument_initialize(quxlang::argument_init_input{
                                                        .from = ref_source,
                                                        .to = parse_type_symbol("CONST& TT(t)"),
                                                        .adaptations = quxlang::allowed_adaptations::destination_rebinding,
                                                    },
                                                    std::nullopt);
    ASSERT_TRUE(tt_const.has_value());
    EXPECT_EQ(*tt_const, parse_type_symbol("CONST& I32"));

    auto auto_value = c.get_ensig_argument_initialize(quxlang::argument_init_input{
                                                          .from = ref_source,
                                                          .to = parse_type_symbol("AUTO(t)"),
                                                          .adaptations = quxlang::allowed_adaptations::destination_rebinding,
                                                      },
                                                      std::nullopt);
    ASSERT_TRUE(auto_value.has_value());
    EXPECT_EQ(*auto_value, parse_type_symbol("I32"));
}

TEST(quxlang, class_conversion_uses_effective_source_forms_and_final_const_materialization)
{
    auto c = make_main_module_compiler(R"(
::from_const_i32 CLASS
{
    .value VAR I32;

    .CONSTRUCTOR FUNCTION(@OTHER CONST& I32)
    {
        .value := OTHER;
    }
}
)");

    auto class_type = parse_type_symbol("MODULE(main)::from_const_i32");
    auto const_ref_class_type = parse_type_symbol("CONST& MODULE(main)::from_const_i32");

    auto by_value = c.get_convertible_by_call(quxlang::implicitly_convertible_to_input{
                                                  .from = parse_type_symbol("I32"),
                                                  .to = class_type,
                                              },
                                              std::nullopt);
    ASSERT_TRUE(by_value.has_value());
    EXPECT_EQ(*by_value, class_type);

    auto by_const_ref = c.get_convertible_by_call(quxlang::implicitly_convertible_to_input{
                                                      .from = parse_type_symbol("I32"),
                                                      .to = const_ref_class_type,
                                                  },
                                                  std::nullopt);
    ASSERT_TRUE(by_const_ref.has_value());
    EXPECT_EQ(*by_const_ref, const_ref_class_type);
}

TEST(quxlang, constexpr_result_bool)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    test_querygraph_compiler c(sources, "linux-x64");

    auto get_constexpr_bool = [&](std::string expr_string) -> bool
    {
        quxlang::expression expr = parse_expression_text(expr_string);
        auto tempfile = temp_output_file();
        bool yaynay;
        try
        {
            yaynay = c.get_constexpr_bool(quxlang::constexpr_input{.expr = expr, .context = mainmodule}, tempfile);
        } catch (...)
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "tempfile: " << tempfile << std::endl;
            }
            throw;
        }
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "tempfile: " << tempfile << std::endl;
        }
        return yaynay;
    };
    auto val1 = get_constexpr_bool("2 + I32(@OTHER 8) - 4 < 5");
    ASSERT_FALSE(val1);
    auto val2 = get_constexpr_bool("2 + I32(@OTHER 3) - 4 < 5");
    ASSERT_TRUE(val2);
    ASSERT_TRUE(get_constexpr_bool("TRUE || FALSE"));
    ASSERT_TRUE(get_constexpr_bool("(TRUE &! TRUE) == FALSE"));
    ASSERT_TRUE(get_constexpr_bool("(TRUE |! FALSE) == FALSE"));
    ASSERT_TRUE(get_constexpr_bool("(TRUE ^! TRUE) == TRUE"));
    ASSERT_TRUE(get_constexpr_bool("(TRUE ^> FALSE) == FALSE"));
    ASSERT_TRUE(get_constexpr_bool("(FALSE ^> FALSE) == TRUE"));
    ASSERT_TRUE(get_constexpr_bool("(FALSE ^< TRUE) == FALSE"));
    ASSERT_TRUE(get_constexpr_bool("(FALSE !!) == TRUE"));
    ASSERT_TRUE(get_constexpr_bool("FALSE < TRUE"));
    ASSERT_TRUE(get_constexpr_bool("TRUE > FALSE"));
    ASSERT_TRUE(get_constexpr_bool("FALSE <= TRUE"));
    ASSERT_TRUE(get_constexpr_bool("TRUE >= FALSE"));
    ASSERT_TRUE(get_constexpr_bool("FALSE <= FALSE"));
    ASSERT_TRUE(get_constexpr_bool("TRUE >= TRUE"));
    ASSERT_TRUE(get_constexpr_bool("(FALSE < TRUE) == (TRUE > FALSE)"));
    ASSERT_TRUE(get_constexpr_bool("(BYTE(@OTHER 6) #^> BYTE(@OTHER 3)) == BYTE(@OTHER 251)"));
    ASSERT_TRUE(get_constexpr_bool("(BYTE(@OTHER 6) #^< BYTE(@OTHER 3)) == BYTE(@OTHER 254)"));
    ASSERT_FALSE(get_constexpr_bool("IS_INTEGRAL(BYTE)"));
    ASSERT_TRUE(get_constexpr_bool("IS_INTEGRAL(U8)"));
    ASSERT_TRUE(get_constexpr_bool("SAME_TYPES(BYTE, BYTE)"));
    ASSERT_FALSE(get_constexpr_bool("SAME_TYPES(BYTE, U8)"));
}

TEST(quxlang, byte_and_u8_are_not_implicitly_interchangeable)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    test_querygraph_compiler c(sources, "linux-x64");

    auto byte = parse_type_symbol("BYTE");
    auto u8 = parse_type_symbol("U8");

    auto u8_ctor = quxlang::initialization_reference{
        .initializee = quxlang::submember{.of = u8, .name = "CONSTRUCTOR"},
        .parameters = quxlang::instatype_from_invotype(quxlang::invotype{.named = {{"THIS", quxlang::create_nslot(u8)}, {"OTHER", byte}}}),
        .adaptations = quxlang::allowed_adaptations::source_rebinding,
    };
    EXPECT_FALSE(c.get_instanciation(u8_ctor, std::nullopt).has_value());

    auto byte_ctor = quxlang::initialization_reference{
        .initializee = quxlang::submember{.of = byte, .name = "CONSTRUCTOR"},
        .parameters = quxlang::instatype_from_invotype(quxlang::invotype{.named = {{"THIS", quxlang::create_nslot(byte)}, {"OTHER", u8}}}),
        .adaptations = quxlang::allowed_adaptations::source_rebinding,
    };
    EXPECT_FALSE(c.get_instanciation(byte_ctor, std::nullopt).has_value());

    auto explicit_u8_ctor = quxlang::initialization_reference{
        .initializee = quxlang::submember{.of = u8, .name = "CONSTRUCTOR"},
        .parameters = quxlang::instatype_from_invotype(quxlang::invotype{.named = {{"THIS", quxlang::create_nslot(u8)}, {"EXPLICIT", byte}}}),
        .adaptations = quxlang::allowed_adaptations::destination_rebinding,
    };
    EXPECT_TRUE(c.get_instanciation(explicit_u8_ctor, std::nullopt).has_value());

    auto explicit_byte_ctor = quxlang::initialization_reference{
        .initializee = quxlang::submember{.of = byte, .name = "CONSTRUCTOR"},
        .parameters = quxlang::instatype_from_invotype(quxlang::invotype{.named = {{"THIS", quxlang::create_nslot(byte)}, {"EXPLICIT", u8}}}),
        .adaptations = quxlang::allowed_adaptations::destination_rebinding,
    };
    EXPECT_TRUE(c.get_instanciation(explicit_byte_ctor, std::nullopt).has_value());
}

TEST(quxlang, user_constructor_other_same_type_is_rejected)
{
    quxlang::source_bundle sources;

    quxlang::target_configuration target;
    target.module_configurations["main"] = quxlang::module_configuration{.source = "main"};
    sources.targets["linux-x64"] = target;

    quxlang::module_source main_module;
    main_module.files["main.qx"] = quxlang::source_file{.contents = R"(
::bad_ctor CLASS
{
    .CONSTRUCTOR FUNCTION(@OTHER bad_ctor)
    {
    }
}
)"};
    sources.module_sources["main"] = main_module;

    test_querygraph_compiler c(sources, "linux-x64");

    auto bad_ctor_symbol = quxlang::submember{
        .of = quxlang::subsymbol{.of = quxlang::absolute_module_reference{"main"}, .name = "bad_ctor"},
        .name = "CONSTRUCTOR",
    };

    EXPECT_THROW(c.get_functum_user_overloads(bad_ctor_symbol, std::nullopt), std::logic_error);
}

TEST(builtin_state, user_func_builtin)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    test_querygraph_compiler c(sources, "linux-x64");
    auto type = parse_type_symbol("MODULE(main)::buz::.CONSTRUCTOR #[@THIS NEW& MODULE(main)::buz]");

    auto val_builtin = c.get_function_builtin(type.get_as<quxlang::temploid_reference>(), std::nullopt);
    GTEST_ASSERT_FALSE(val_builtin);
}

TEST(quxlang, constexpr_call_func)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    test_querygraph_compiler c(sources, "linux-x64");

    auto get_constexpr_bool = [&](std::string expr_string) -> bool
    {
        quxlang::expression expr = parse_expression_text(expr_string);
        auto tempfile = temp_output_file();
        bool yaynay;
        try
        {
            yaynay = c.get_constexpr_bool(quxlang::constexpr_input{.expr = expr, .context = mainmodule}, tempfile);
        } catch (...)
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "tempfile: " << tempfile << std::endl;
            }
            throw;
        }
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "tempfile: " << tempfile << std::endl;
        }
        return yaynay;
    };
    auto val1 = get_constexpr_bool("biz(4, 3) == 4");
    ASSERT_FALSE(val1);
    auto val2 = get_constexpr_bool("boq() == 5");
    EXPECT_TRUE(val2);
    auto val3 = get_constexpr_bool("biz(4, 3) == 19");
    ASSERT_TRUE(val3);
    auto val4 = get_constexpr_bool("mif() == 10");
    ASSERT_TRUE(val4);
    auto val5 = get_constexpr_bool("pinc_helper() == 2");
    ASSERT_TRUE(val5);

    auto val6 = get_constexpr_bool("arch_int() == 2");
    ASSERT_TRUE(val6);

    auto val7 = get_constexpr_bool("yip() == 8");
    ASSERT_TRUE(val7);

    auto val8 = get_constexpr_bool("storage_buz_helper() == 10");
    ASSERT_TRUE(val8);

    auto val9 = get_constexpr_bool("aligned_storage_buz_helper() == 9");
    ASSERT_TRUE(val9);


}


TEST(quxlang, constexpr_call_func_arm)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});
    test_querygraph_compiler c(sources, "linux-arm64");

    auto get_constexpr_bool = [&](std::string expr_string) -> bool
    {
        quxlang::expression expr = parse_expression_text(expr_string);
        auto tempfile = temp_output_file();
        bool yaynay;
        try
        {
            yaynay = c.get_constexpr_bool(quxlang::constexpr_input{.expr = expr, .context = mainmodule}, tempfile);
        } catch (...)
        {
            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "tempfile: " << tempfile << std::endl;
            }
            throw;
        }
        if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
        {
            std::cout << "tempfile: " << tempfile << std::endl;
        }
        return yaynay;
    };
    auto val1 = get_constexpr_bool("biz(4, 3) == 4");
    ASSERT_FALSE(val1);
    auto val2 = get_constexpr_bool("boq() == 5");
    EXPECT_TRUE(val2);
    auto val3 = get_constexpr_bool("biz(4, 3) == 19");
    ASSERT_TRUE(val3);
    auto val4 = get_constexpr_bool("mif() == 10");
    ASSERT_TRUE(val4);
    auto val5 = get_constexpr_bool("pinc_helper() == 2");
    ASSERT_TRUE(val5);

    auto val6 = get_constexpr_bool("arch_int() == 1");
    ASSERT_TRUE(val6);

    auto val7 = get_constexpr_bool("storage_buz_helper() == 10");
    ASSERT_TRUE(val7);

    auto val8 = get_constexpr_bool("aligned_storage_buz_helper() == 9");
    ASSERT_TRUE(val8);


}

TEST(quxlang, func_gen)
{

    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;

    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});

    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});

    test_querygraph_compiler c(sources, "linux-x64");

    auto func_name = parse_type_symbol("MODULE(main)::biz #{I32, I32}");

    func_name = quxlang::with_context(func_name, mainmodule);

    std::string out_type_name = func_name.type().name();

    quxlang::instanciation_reference func_name_real = func_name.template get_as<quxlang::instanciation_reference>();

    auto tempname = temp_output_file();
    auto func = c.get_vm_procedure3(func_name_real, tempname);

    std::string result = quxlang::vmir2::assembler(func).to_string(func);

    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        std::cout << result << std::endl;
    }
}

TEST(quxlang, datatype_struct_equality_builtin_presence)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    test_querygraph_compiler c(sources, "linux-x64");

    auto datatype_eq = c.get_functum_builtin_overloads(
        parse_type_symbol("MODULE(main)::datatype_equality_probe::.OPERATOR=="), std::nullopt);
    auto datatype_ne = c.get_functum_builtin_overloads(
        parse_type_symbol("MODULE(main)::datatype_equality_probe::.OPERATOR!="), std::nullopt);
    auto nondatatype_eq = c.get_functum_builtin_overloads(
        parse_type_symbol("MODULE(main)::nondatatype_equality_probe::.OPERATOR=="), std::nullopt);
    auto nondatatype_ne = c.get_functum_builtin_overloads(
        parse_type_symbol("MODULE(main)::nondatatype_equality_probe::.OPERATOR!="), std::nullopt);

    EXPECT_FALSE(datatype_eq.empty());
    EXPECT_FALSE(datatype_ne.empty());
    EXPECT_TRUE(nondatatype_eq.empty());
    EXPECT_TRUE(nondatatype_ne.empty());
}

TEST(quxlang, positional_pack_invalid_body_generation)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});

    test_querygraph_compiler c(sources, "linux-x64");

    auto expect_compile_failure = [&](std::string const& function_name)
    {
        auto func_name = parse_type_symbol("MODULE(main)::" + function_name + " #{I32}");
        func_name = quxlang::with_context(func_name, mainmodule);
        auto tempname = temp_output_file();
        EXPECT_THROW(c.get_vm_procedure3(func_name.get_as< quxlang::instanciation_reference >(), tempname), std::logic_error);
    };

    expect_compile_failure("pack_arg_out_of_range");
    expect_compile_failure("pack_direct_use");
}

TEST(quxlang, storage_type_lookup)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});

    test_querygraph_compiler c(sources, "linux-x64");

    auto resolved = c.get_lookup(quxlang::contextual_type_reference{
        .context = mainmodule,
        .type = parse_type_symbol("STORAGE(buz, yak)"),
    }, std::nullopt);

    ASSERT_TRUE(resolved.has_value());
    ASSERT_EQ(quxlang::to_string(*resolved), "STORAGE(MODULE(main)::buz, MODULE(main)::yak)");
}

TEST(quxlang, constexpr_allocator_builtin_lookup_and_overloads)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});

    test_querygraph_compiler c(sources, "linux-x64");

    auto parsed = parse_expression_text("CONSTEXPR_ALLOC#(I32)");
    ASSERT_TRUE(parsed.type_is< quxlang::expression_symbol_reference >());
    auto input = parsed.get_as< quxlang::expression_symbol_reference >().symbol;
    ASSERT_TRUE(input.type_is< quxlang::initialization_reference >());

    auto resolved = c.get_lookup(quxlang::contextual_type_reference{
        .context = mainmodule,
        .type = input,
    }, std::nullopt);

    ASSERT_TRUE(resolved.has_value());
    ASSERT_TRUE(resolved->type_is< quxlang::instanciation_reference >());
    EXPECT_EQ(quxlang::to_string(*resolved), "CONSTEXPR_ALLOC#{I32}");
    EXPECT_EQ(c.get_symbol_type(*resolved, std::nullopt), quxlang::symbol_kind::functum);

    auto overloads = c.get_functum_builtin_overloads(*resolved, std::nullopt);
    ASSERT_EQ(overloads.size(), 1);
    auto const& overload = *overloads.begin();
    EXPECT_TRUE(overload.interface.positional.empty());
    EXPECT_TRUE(overload.interface.named.empty());

    auto canonical_callee = c.get_lookup(quxlang::contextual_type_reference{
        .context = mainmodule,
        .type = as< quxlang::initialization_reference >(input).initializee,
    }, std::nullopt);
    ASSERT_TRUE(canonical_callee.has_value());

    auto canonical_select_input = as< quxlang::initialization_reference >(input);
    canonical_select_input.context = mainmodule;
    canonical_select_input.initializee = *canonical_callee;

    auto selected_template = c.get_templex_select_template(std::move(canonical_select_input), std::nullopt);
    ASSERT_TRUE(selected_template.has_value());
    EXPECT_EQ(c.get_symbol_type(*selected_template, std::nullopt), quxlang::symbol_kind::template_);
    EXPECT_TRUE(c.get_template_builtin(*selected_template, std::nullopt));
    EXPECT_FALSE(c.get_function_builtin(*selected_template, std::nullopt));

    auto selected = quxlang::as< quxlang::instanciation_reference >(*resolved).temploid;
    EXPECT_EQ(selected, *selected_template);
}

TEST(quxlang, storage_type_validation)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    auto sources = quxlang::load_bundle_sources_for_targets(testdata / "example", {});
    auto mainmodule = quxlang::with_context(quxlang::context_reference{}, quxlang::absolute_module_reference{"main"});

    test_querygraph_compiler c(sources, "linux-x64");

    auto expect_compile_failure = [&](std::string const& function_name)
    {
        auto func_name = parse_type_symbol("MODULE(main)::" + function_name + " #{}");
        func_name = quxlang::with_context(func_name, mainmodule);
        auto tempname = temp_output_file();
        EXPECT_THROW(c.get_vm_procedure3(func_name.get_as< quxlang::instanciation_reference >(), tempname), std::logic_error);
    };

    expect_compile_failure("aligned_storage_baz");
    expect_compile_failure("storage_wrong_type_baz");
}

#include <quxlang/fixed_bytemath.hpp>

TEST(fixed_bytemath, unlimited_to_fixed_overflow_undefined) {
  using namespace quxlang::bytemath;

  // Case 1: No overflow
  {
    fixed_int_options opt;
    opt.bits = 8;
    opt.has_sign = false;
    opt.overflow_undefined = true;

    sle_int_unlimited input;
    input.is_negative = false;
    input.data = {std::byte(10)};

    auto res = unlimited_to_fixed(opt, input);
    ASSERT_FALSE(res.result_is_undefined);
    ASSERT_EQ(res.data_bytes.size(), 1);
    ASSERT_EQ(res.data_bytes[0], std::byte(10));
  }

  // Case 2a: 255 to 8 bits unsigned
  {
    fixed_int_options opt;
    opt.bits = 8;
    opt.has_sign = false;
    opt.overflow_undefined = true;

    sle_int_unlimited input;
    input.is_negative = false;
    input.data = {std::byte(255)}; // 255

    auto res = unlimited_to_fixed(opt, input);
    ASSERT_FALSE(res.result_is_undefined);
    ASSERT_EQ(res.data_bytes.size(), 1);
    ASSERT_EQ(res.data_bytes[0], std::byte(255));
  }

  // Case 2: Overflow with overflow_undefined = true
  {
    fixed_int_options opt;
    opt.bits = 8;
    opt.has_sign = false;
    opt.overflow_undefined = true;

    sle_int_unlimited input;
    input.is_negative = false;
    input.data = {std::byte(0), std::byte(1)}; // 256, fits in 9 bits, not 8

    auto res = unlimited_to_fixed(opt, input);
    ASSERT_TRUE(res.result_is_undefined);
  }

  // Case 3: Overflow with overflow_undefined = false (Truncation)
  {
    fixed_int_options opt;
    opt.bits = 8;
    opt.has_sign = false;
    opt.overflow_undefined = false;

    sle_int_unlimited input;
    input.is_negative = false;
    input.data = {std::byte(10), std::byte(1)}; // 266 = 256 + 10

    auto res = unlimited_to_fixed(opt, input);
    ASSERT_FALSE(res.result_is_undefined);
    ASSERT_EQ(res.data_bytes.size(), 1);
    ASSERT_EQ(res.data_bytes[0], std::byte(10)); // Truncated to 10
  }

  // Case 4: Signed extension
  {
    fixed_int_options opt;
    opt.bits = 16;
    opt.has_sign = true;
    opt.overflow_undefined = false;

    sle_int_unlimited input;
    input.is_negative = true;
    input.data = {std::byte(1)}; // -1

    auto res = unlimited_to_fixed(opt, input);
    ASSERT_FALSE(res.result_is_undefined);
    ASSERT_EQ(res.data_bytes.size(), 2);
    ASSERT_EQ(res.data_bytes[0], std::byte(0xFF));
    ASSERT_EQ(res.data_bytes[1], std::byte(0xFF));
  }
}

TEST(fixed_bytemath, fixed_int_sub_wraps_signed_values) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_sub_le(opt, {std::byte{0x80}}, {std::byte{0x01}});
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x7F}}));
}

TEST(fixed_bytemath, fixed_int_add_wraps_signed_values) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_add_le(opt, {std::byte{0x7F}}, {std::byte{0x01}});
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x80}}));
}

TEST(fixed_bytemath, fixed_int_add_wraps_unsigned_values) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = false;

  auto res = fixed_int_add_le(opt, {std::byte{0xFF}}, {std::byte{0x01}});
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x00}}));
}

TEST(fixed_bytemath, fixed_int_mul_wraps_signed_values) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_mul_le(opt, {std::byte{0x40}}, {std::byte{0x04}});
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x00}}));
}

TEST(fixed_bytemath, fixed_int_div_signed_truncates_toward_zero) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_div_le(opt, {std::byte{0xF9}}, {std::byte{0x02}}); // -7 / 2
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0xFD}})); // -3
}

TEST(fixed_bytemath, fixed_int_div_zero_is_undefined) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_div_le(opt, {std::byte{0x0A}}, {std::byte{0x00}});
  ASSERT_TRUE(res.result_is_undefined);
}

TEST(fixed_bytemath, fixed_int_mod_negative_modulus_is_undefined) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_mod_le(opt, {std::byte{0x07}}, {std::byte{0xFF}});
  ASSERT_TRUE(res.result_is_undefined);
}

TEST(fixed_bytemath, fixed_int_shift_left_detects_overflow) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = true;

  auto res = fixed_int_shift_up_le(opt, {std::byte{0x01}}, 8);
  ASSERT_TRUE(res.result_is_undefined);
}

TEST(fixed_bytemath, fixed_int_shift_right_detects_overflow) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = true;

  auto res = fixed_int_shift_down_le(opt, {std::byte{0x80}}, 8);
  ASSERT_TRUE(res.result_is_undefined);
}

TEST(fixed_bytemath, fixed_int_shift_left_can_define_overflow_as_zero) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = false;

  auto res = fixed_int_shift_up_le(opt, {std::byte{0x01}}, 8);
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x00}}));
}

TEST(fixed_bytemath, fixed_int_shift_right_can_define_overflow_as_zero) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = false;

  auto res = fixed_int_shift_down_le(opt, {std::byte{0x80}}, 8);
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x00}}));
}

TEST(fixed_bytemath, fixed_int_shift_left_truncates_when_in_range) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = true;

  auto res = fixed_int_shift_up_le(opt, {std::byte{0x81}}, 1);
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x02}}));
}

TEST(fixed_bytemath, fixed_int_shift_right_preserves_in_range_result) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = false;
  opt.overflow_undefined = true;

  auto res = fixed_int_shift_down_le(opt, {std::byte{0x81}}, 1);
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0x40}}));
}

TEST(fixed_bytemath, fixed_int_mod_signed_remainder_matches_dividend_sign) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto res = fixed_int_mod_le(opt, {std::byte{0xF9}}, {std::byte{0x02}}); // -7 % 2
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0xFF}})); // -1
}

TEST(fixed_bytemath, fixed_int_convert_sign_extends_negative_values) {
  using namespace quxlang::bytemath;

  fixed_int_options from_opt;
  from_opt.bits = 8;
  from_opt.has_sign = true;
  from_opt.overflow_undefined = false;

  fixed_int_options to_opt;
  to_opt.bits = 16;
  to_opt.has_sign = true;
  to_opt.overflow_undefined = false;

  auto res = fixed_int_convert(from_opt, to_opt, {std::byte{0xFF}});
  ASSERT_FALSE(res.result_is_undefined);
  ASSERT_EQ(res.data_bytes, std::vector< std::byte >({std::byte{0xFF}, std::byte{0xFF}}));
}

TEST(fixed_bytemath, fixed_int_convert_non_byte_width_round_trip) {
  using namespace quxlang::bytemath;

  fixed_int_options from_opt;
  from_opt.bits = 5;
  from_opt.has_sign = true;
  from_opt.overflow_undefined = false;

  fixed_int_options to_opt;
  to_opt.bits = 9;
  to_opt.has_sign = true;
  to_opt.overflow_undefined = false;

  auto widened = fixed_int_convert(from_opt, to_opt, {std::byte{0x1F}}); // -1 in 5 bits
  ASSERT_FALSE(widened.result_is_undefined);
  ASSERT_EQ(widened.data_bytes, std::vector< std::byte >({std::byte{0xFF}, std::byte{0x01}}));

  auto narrowed = fixed_int_convert(to_opt, from_opt, widened.data_bytes);
  ASSERT_FALSE(narrowed.result_is_undefined);
  ASSERT_EQ(narrowed.data_bytes, std::vector< std::byte >({std::byte{0x1F}}));
}

TEST(fixed_bytemath, le_int_fixed_to_unlimited_handles_signed_minimum) {
  using namespace quxlang::bytemath;

  fixed_int_options opt;
  opt.bits = 8;
  opt.has_sign = true;
  opt.overflow_undefined = false;

  auto value = le_int_fixed_to_unlimited(opt, {std::byte{0x80}});
  ASSERT_TRUE(value.is_negative);
  ASSERT_EQ(value.data, std::vector< std::byte >({std::byte{0x80}}));
}
