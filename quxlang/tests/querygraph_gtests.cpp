// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <gtest/gtest.h>

#include <quxlang/compiler_querygraph.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/llvm-backend-types.hpp>
#include <quxlang/manipulators/typeutils.hpp>
#include <quxlang/parsers/parse_type_symbol.hpp>
#include <quxlang/parsers/parse_function_block.hpp>
#include <quxlang/source_loader.hpp>
#include <quxlang/queries/argument_adaptation_is_better_fit.hpp>
#include <quxlang/queries/argument_adaptation_rank.hpp>
#include <quxlang/queries/antestatal_static_value.hpp>
#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>
#include <quxlang/queries/constexpr_routine.hpp>
#include <quxlang/queries/constexpr_routine_v3.hpp>
#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/declaroids.hpp>
#include <quxlang/queries/enum_info.hpp>
#include <quxlang/queries/flagset_info.hpp>
#include <quxlang/queries/fusion_layout.hpp>
#include <quxlang/queries/union_info.hpp>
#include <quxlang/queries/variant_info.hpp>
#include <quxlang/queries/global_init_type.hpp>
#include <quxlang/queries/global_is_antestatal_static.hpp>
#include <quxlang/queries/global_is_per_thread.hpp>
#include <quxlang/queries/global_is_serialoid_static.hpp>
#include <quxlang/queries/implementation_function_map.hpp>
#include <quxlang/queries/implementation_interface_type.hpp>
#include <quxlang/queries/machine_info.hpp>
#include <quxlang/queries/module_ast.hpp>
#include <quxlang/queries/module_options_map.hpp>
#include <quxlang/queries/module_source_name.hpp>
#include <quxlang/queries/module_source_name_map.hpp>
#include <quxlang/queries/module_sources.hpp>
#include <quxlang/queries/output_binaries_information.hpp>
#include <quxlang/queries/output_binary_information.hpp>
#include <quxlang/queries/output_llvm_backend_options.hpp>
#include <quxlang/queries/output_llvm_input.hpp>
#include <quxlang/queries/output_unoptimized_llvm.hpp>
#include <quxlang/queries/output_list.hpp>
#include <quxlang/queries/instanciation.hpp>
#include <quxlang/queries/interface_defaultable.hpp>
#include <quxlang/queries/interface_slot_list.hpp>
#include <quxlang/queries/list_static_tests.hpp>
#include <quxlang/queries/list_unit_tests.hpp>
#include <quxlang/queries/lookup.hpp>
#include <quxlang/queries/source_bundle.hpp>
#include <quxlang/queries/source_file_id.hpp>
#include <quxlang/queries/source_file_index.hpp>
#include <quxlang/queries/source_file_name.hpp>
#include <quxlang/queries/symboid.hpp>
#include <quxlang/queries/symboid_subdeclaroids.hpp>
#include <quxlang/queries/symbol_type.hpp>
#include <quxlang/queries/class_type.hpp>
#include <quxlang/queries/target_backend.hpp>
#include <quxlang/queries/target_llvm_backend_options.hpp>
#include <quxlang/queries/test_is_enabled_for_static_testing.hpp>
#include <quxlang/queries/test_is_enabled_for_unit_testing.hpp>
#include <quxlang/queries/type_is_antestatal.hpp>
#include <quxlang/queries/type_is_serialoid.hpp>
#include <quxlang/queries/type_is_stringlike.hpp>
#include <quxlang/queries/type_is_trivially_default_constructible.hpp>
#include <quxlang/queries/type_is_trivially_relocatable.hpp>
#include <quxlang/queries/class_placement_info.hpp>
#include <quxlang/queries/user_deserialize_exists.hpp>
#include <quxlang/queries/variable_type.hpp>
#include <quxlang/queries/vm_procedure3.hpp>
#include <quxlang/queries/vmir_dependencies.hpp>
#include <quxlang/parsers/vmir2.hpp>
#include <quxlang/vmir2/assembler.hpp>
#include <quxlang/vmir2/routine_requirements.hpp>
#include <quxlang/vmir2/vmir2.hpp>
#include "graph_dump_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <variant>

namespace
{
    auto with_test_language_declaration(std::string contents) -> std::string
    {
        return "LANGUAGE QUXLANG EN 0.0;\n\n" + std::move(contents);
    }

    auto parse_type_symbol_text(std::string const& text) -> quxlang::type_symbol
    {
        quxlang::parsers::parsing_context ctx = quxlang::parsers::make_unlocated_parsing_context(text);
        ctx.allow_internal_subtag_symbols = true;
        quxlang::type_symbol result = quxlang::parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::syntax_compilation_error("Input not fully parsed");
        }
        return result;
    }

    /// Counts non-overlapping substring occurrences in generated text.
    auto count_substrings(std::string const& text, std::string const& needle) -> std::size_t
    {
        std::size_t count = 0;
        std::size_t position = text.find(needle);
        while (position != std::string::npos)
        {
            ++count;
            position = text.find(needle, position + needle.size());
        }
        return count;
    }

    auto make_test_source_bundle() -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle;

        quxlang::target_configuration x64;
        x64.target_output_config.cpu_type = quxlang::cpu::x86_64;
        x64.target_output_config.os_type = quxlang::os::linux;
        x64.target_output_config.binary_type = quxlang::binary::elf;
        x64.module_configurations["main"].source = "main_x64";
        x64.module_configurations["util"].source = "util_shared";
        x64.outputs = std::map< std::string, quxlang::output_config >{
            {"app", quxlang::output_config{.type = quxlang::output_kind::executable, .module = "main", .main_functanoid = "main"}},
            {"util", quxlang::output_config{.type = quxlang::output_kind::shared_library, .module = "util", .main_functanoid = std::nullopt}},
        };

        quxlang::target_configuration arm64;
        arm64.target_output_config.cpu_type = quxlang::cpu::arm_64;
        arm64.target_output_config.os_type = quxlang::os::macos;
        arm64.target_output_config.binary_type = quxlang::binary::macho;
        arm64.module_configurations["main"].source = "main_arm64";
        arm64.module_configurations["util"].source = "util_shared";
        arm64.outputs = std::map< std::string, quxlang::output_config >{
            {"ios_app", quxlang::output_config{.type = quxlang::output_kind::executable, .module = "main", .main_functanoid = "main"}},
        };

        bundle.targets["x64"] = x64;
        bundle.targets["arm64"] = arm64;

        bundle.module_sources["main_x64"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I32;")};
        bundle.module_sources["main_arm64"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I64;")};
        bundle.module_sources["util_shared"].files["util.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::util VAR I32;")};

        return bundle;
    }

    auto make_single_main_source_bundle(std::string contents) -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle;

        quxlang::target_configuration x64;
        x64.target_output_config.cpu_type = quxlang::cpu::x86_64;
        x64.target_output_config.os_type = quxlang::os::linux;
        x64.target_output_config.binary_type = quxlang::binary::elf;
        x64.module_configurations["main"].source = "main_x64";

        bundle.targets["x64"] = x64;
        bundle.module_sources["main_x64"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(std::move(contents))};

        return bundle;
    }

    auto make_unit_test_suite_source_bundle(std::string main_contents) -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle(std::move(main_contents));
        quxlang::target_configuration& target = bundle.targets.at("x64");
        target.outputs = std::map< std::string, quxlang::output_config >{
            {"tests", quxlang::output_config{.type = quxlang::output_kind::unit_test_suite, .module = "main"}},
        };
        target.module_configurations["RUNTIME"].source = "runtime_x64";
        bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::ASSERT_FAIL FUNCTION(@expr STRING_CONSTANT, @file SZ, @line SZ, @column SZ, @tag CONST -> STRING_CONSTANT)
{
}

::UNIT_TESTING_PROGRAM_START ASM_PROCEDURE X64
{
  MOVABS RAX, OFFSET OBJECT_REF(UNIT_TEST_COUNT)
  MOVABS RBX, OFFSET OBJECT_REF(UNIT_TEST_NAMES)
  MOVABS RCX, OFFSET OBJECT_REF(UNIT_TEST_PROC)
  RET
}
)QX")};
        return bundle;
    }

    auto make_x64_graph(quxlang::source_bundle const& bundle) -> quxlang::compiler_querygraph
    {
        return quxlang::compiler_querygraph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                            quxlang::tests::current_test_graph_dump_path());
    }

    /**
     * Builds a one-module source bundle targeting the requested CPU.
     */
    auto make_single_main_source_bundle_for_cpu(std::string contents, quxlang::cpu cpu_type) -> quxlang::source_bundle
    {
        quxlang::source_bundle bundle;

        quxlang::target_configuration target;
        target.target_output_config.cpu_type = cpu_type;
        target.target_output_config.os_type = quxlang::os::linux;
        target.target_output_config.binary_type = quxlang::binary::elf;
        target.module_configurations["main"].source = "main";

        bundle.targets["target"] = target;
        bundle.module_sources["main"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(std::move(contents))};

        return bundle;
    }

    /// Builds an I32 constexpr value with the requested low byte.
    auto test_i32_value(std::byte low_byte) -> quxlang::constexpr_value
    {
        return quxlang::antestatal_value(quxlang::antestatal_primitive{.value = {low_byte, std::byte{0}, std::byte{0}, std::byte{0}}});
    }

    /// Checks that a constexpr value is the expected I32 primitive.
    auto assert_i32_value(quxlang::constexpr_value const& value, std::byte low_byte) -> void
    {
        auto const& antestatal = quxlang::constexpr_value_as_antestatal(value);
        ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(antestatal));
        ASSERT_EQ(quxlang::as< quxlang::antestatal_primitive >(antestatal).value, (std::vector{low_byte, std::byte{0}, std::byte{0}, std::byte{0}}));
    }
} // namespace

TEST(querygraph_queries, source_bundle_returns_injected_bundle)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::source_bundle_query >(std::monostate{});

    ASSERT_TRUE(resolved.targets.contains("x64"));
    ASSERT_TRUE(resolved.targets.contains("arm64"));
    ASSERT_TRUE(resolved.module_sources.contains("main_x64"));
    ASSERT_EQ(resolved.module_sources.at("main_x64").files.at("main.qxs").get().contents, with_test_language_declaration("::main VAR I32;"));
}

TEST(querygraph_queries, machine_info_returns_injected_output_info)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::machine_info_query >(std::monostate{});

    ASSERT_EQ(resolved.cpu_type, quxlang::cpu::x86_64);
    ASSERT_EQ(resolved.os_type, quxlang::os::linux);
    ASSERT_EQ(resolved.binary_type, quxlang::binary::elf);
}

TEST(querygraph_queries, binary_keywords_reflect_elf_machine_binary_type)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    EXPECT_TRUE(graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_value_keyword{"BINARY_ELF"},
        .context = context,
    }));
    EXPECT_FALSE(graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_value_keyword{"BINARY_MACHO"},
        .context = context,
    }));
}

TEST(querygraph_queries, binary_keywords_reflect_macho_machine_binary_type)
{
    quxlang::source_bundle bundle;
    quxlang::target_configuration target;
    target.target_output_config.cpu_type = quxlang::cpu::arm_64;
    target.target_output_config.os_type = quxlang::os::macos;
    target.target_output_config.binary_type = quxlang::binary::macho;
    target.module_configurations["main"].source = "main_macho";

    bundle.targets["macho"] = target;
    bundle.module_sources["main_macho"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I32;")};

    quxlang::compiler_querygraph graph(bundle, "macho", bundle.targets.at("macho").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    EXPECT_TRUE(graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_value_keyword{"BINARY_MACHO"},
        .context = context,
    }));
    EXPECT_FALSE(graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_value_keyword{"BINARY_ELF"},
        .context = context,
    }));
}

TEST(querygraph_queries, binary_keywords_filter_include_if_declarations)
{
    auto bundle = make_single_main_source_bundle(R"QX(
::selected INCLUDE_IF(BINARY_ELF) VAR I32;
::filtered INCLUDE_IF(BINARY_MACHO) VAR I32;
)QX");
    auto graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const selected = quxlang::type_symbol(quxlang::subsymbol{main, "selected"});
    quxlang::type_symbol const filtered = quxlang::type_symbol(quxlang::subsymbol{main, "filtered"});

    EXPECT_EQ(graph.make_request< quxlang::declaroids_query >(selected).size(), 1);
    EXPECT_TRUE(graph.make_request< quxlang::declaroids_query >(filtered).empty());
}

TEST(querygraph_queries, architecture_keywords_reflect_machine_info)
{
    auto evaluate = [](quxlang::cpu cpu_type, std::string const& keyword) -> bool
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle_for_cpu("::main VAR I32;", cpu_type);
        quxlang::compiler_querygraph graph(bundle, "target", bundle.targets.at("target").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
        quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
        return graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
            .expr = quxlang::expression_value_keyword{.keyword = keyword},
            .context = context,
        });
    };

    EXPECT_TRUE(evaluate(quxlang::cpu::x86_64, "ARCH_IS_X64"));
    EXPECT_FALSE(evaluate(quxlang::cpu::x86_64, "ARCH_IS_X86"));
    EXPECT_TRUE(evaluate(quxlang::cpu::x86_32, "ARCH_IS_X86"));
    EXPECT_FALSE(evaluate(quxlang::cpu::x86_32, "ARCH_IS_X64"));
    EXPECT_TRUE(evaluate(quxlang::cpu::arm_32, "ARCH_IS_ARM32"));
    EXPECT_FALSE(evaluate(quxlang::cpu::arm_32, "ARCH_IS_ARM64"));
    EXPECT_TRUE(evaluate(quxlang::cpu::arm_64, "ARCH_IS_ARM64"));
    EXPECT_FALSE(evaluate(quxlang::cpu::arm_64, "ARCH_IS_ARM32"));
    EXPECT_TRUE(evaluate(quxlang::cpu::riscv_64, "ARCH_IS_RISCV64"));
    EXPECT_FALSE(evaluate(quxlang::cpu::x86_64, "ARCH_IS_RISCV64"));
}

TEST(querygraph_queries, environment_keywords_reflect_machine_info)
{
    auto evaluate = [](quxlang::environment environment_type, std::string const& keyword) -> bool
    {
        quxlang::source_bundle bundle;
        quxlang::target_configuration target;
        target.target_output_config.cpu_type = quxlang::cpu::x86_64;
        target.target_output_config.os_type = quxlang::os::linux;
        target.target_output_config.binary_type = quxlang::binary::elf;
        target.target_output_config.environment_type = environment_type;
        target.module_configurations["main"].source = "main_env";

        bundle.targets["target"] = target;
        bundle.module_sources["main_env"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I32;")};

        quxlang::compiler_querygraph graph(bundle, "target", bundle.targets.at("target").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
        quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
        return graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
            .expr = quxlang::expression_value_keyword{.keyword = keyword},
            .context = context,
        });
    };

    EXPECT_TRUE(evaluate(quxlang::environment::static_, "ENVIRONMENT_IS_STATIC"));
    EXPECT_FALSE(evaluate(quxlang::environment::static_, "ENVIRONMENT_IS_GLIBC"));
    EXPECT_TRUE(evaluate(quxlang::environment::glibc, "ENVIRONMENT_IS_GLIBC"));
    EXPECT_FALSE(evaluate(quxlang::environment::glibc, "ENVIRONMENT_IS_STATIC"));
    EXPECT_TRUE(evaluate(quxlang::environment::musl, "ENVIRONMENT_IS_MUSL"));
    EXPECT_TRUE(evaluate(quxlang::environment::bionic, "ENVIRONMENT_IS_BIONIC"));
    EXPECT_TRUE(evaluate(quxlang::environment::msvc, "ENVIRONMENT_IS_MSVC"));
    EXPECT_TRUE(evaluate(quxlang::environment::ucrt, "ENVIRONMENT_IS_UCRT"));
    EXPECT_TRUE(evaluate(quxlang::environment::cygwin, "ENVIRONMENT_IS_CYGWIN"));
    EXPECT_TRUE(evaluate(quxlang::environment::libsystem, "ENVIRONMENT_IS_LIBSYSTEM"));
    EXPECT_TRUE(evaluate(quxlang::environment::freestanding, "ENVIRONMENT_IS_FREESTANDING"));
    EXPECT_FALSE(evaluate(quxlang::environment::glibc, "ENVIRONMENT_IS_FREESTANDING"));
}

TEST(querygraph_queries, asm_procedure_merge_selects_x86_family_architecture)
{
    std::string const source = R"QX(
::entry ASM_PROCEDURE X64
{
  MOV RAX, 64
}

::entry ASM_PROCEDURE X86
{
  MOV EAX, 32
}
)QX";

    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const entry = quxlang::type_symbol(quxlang::subsymbol{main, "entry"});

    quxlang::source_bundle x64_bundle = make_single_main_source_bundle_for_cpu(source, quxlang::cpu::x86_64);
    quxlang::compiler_querygraph x64_graph(x64_bundle, "target", x64_bundle.targets.at("target").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
    quxlang::ast2_symboid const x64_symboid = x64_graph.make_request< quxlang::symboid_query >(entry);
    ASSERT_TRUE(x64_symboid.type_is< quxlang::ast2_asm_procedure_declaration >());
    EXPECT_EQ(x64_symboid.get_as< quxlang::ast2_asm_procedure_declaration >().architecture, "X64");

    quxlang::source_bundle x86_bundle = make_single_main_source_bundle_for_cpu(source, quxlang::cpu::x86_32);
    quxlang::compiler_querygraph x86_graph(x86_bundle, "target", x86_bundle.targets.at("target").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
    quxlang::ast2_symboid const x86_symboid = x86_graph.make_request< quxlang::symboid_query >(entry);
    ASSERT_TRUE(x86_symboid.type_is< quxlang::ast2_asm_procedure_declaration >());
    EXPECT_EQ(x86_symboid.get_as< quxlang::ast2_asm_procedure_declaration >().architecture, "X86");
}

TEST(querygraph_queries, asm_procedure_merge_selects_arm_family_architecture)
{
    std::string const source = R"QX(
::entry ASM_PROCEDURE ARM32
{
  MOV R0, 32
}

::entry ASM_PROCEDURE ARM64
{
  MOV X0, 64
}
)QX";

    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const entry = quxlang::type_symbol(quxlang::subsymbol{main, "entry"});

    quxlang::source_bundle arm32_bundle = make_single_main_source_bundle_for_cpu(source, quxlang::cpu::arm_32);
    quxlang::compiler_querygraph arm32_graph(arm32_bundle, "target", arm32_bundle.targets.at("target").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());
    quxlang::ast2_symboid const arm32_symboid = arm32_graph.make_request< quxlang::symboid_query >(entry);
    ASSERT_TRUE(arm32_symboid.type_is< quxlang::ast2_asm_procedure_declaration >());
    EXPECT_EQ(arm32_symboid.get_as< quxlang::ast2_asm_procedure_declaration >().architecture, "ARM32");

    quxlang::source_bundle arm64_bundle = make_single_main_source_bundle_for_cpu(source, quxlang::cpu::arm_64);
    quxlang::compiler_querygraph arm64_graph(arm64_bundle, "target", arm64_bundle.targets.at("target").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());
    quxlang::ast2_symboid const arm64_symboid = arm64_graph.make_request< quxlang::symboid_query >(entry);
    ASSERT_TRUE(arm64_symboid.type_is< quxlang::ast2_asm_procedure_declaration >());
    EXPECT_EQ(arm64_symboid.get_as< quxlang::ast2_asm_procedure_declaration >().architecture, "ARM64");
}

TEST(querygraph_queries, unwind_format_keywords_reflect_current_linux_elf_codegen_policy)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto evaluate = [&](std::string const& keyword) -> bool
    {
        return graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
            .expr = quxlang::expression_value_keyword{.keyword = keyword},
            .context = context,
        });
    };

    EXPECT_TRUE(evaluate("UNWIND_FORMAT_IS_DWARF_EH_FRAME"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_NONE"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_ARM_EHABI"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_WINDOWS_SEH"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_SJLJ"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_WASM"));
}

TEST(querygraph_queries, unwind_format_keywords_reflect_current_windows_pe_codegen_policy)
{
    quxlang::source_bundle bundle;
    quxlang::target_configuration target;
    target.target_output_config.cpu_type = quxlang::cpu::x86_64;
    target.target_output_config.os_type = quxlang::os::windows;
    target.target_output_config.binary_type = quxlang::binary::pe;
    target.module_configurations["main"].source = "main_windows";

    bundle.targets["windows"] = target;
    bundle.module_sources["main_windows"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I32;")};

    quxlang::compiler_querygraph graph(bundle, "windows", bundle.targets.at("windows").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto evaluate = [&](std::string const& keyword) -> bool
    {
        return graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
            .expr = quxlang::expression_value_keyword{.keyword = keyword},
            .context = context,
        });
    };

    EXPECT_TRUE(evaluate("UNWIND_FORMAT_IS_DWARF_EH_FRAME"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_WINDOWS_SEH"));
}

TEST(querygraph_queries, unwind_format_keywords_reflect_current_arm32_elf_codegen_policy)
{
    quxlang::source_bundle bundle;
    quxlang::target_configuration target;
    target.target_output_config.cpu_type = quxlang::cpu::arm_32;
    target.target_output_config.os_type = quxlang::os::linux;
    target.target_output_config.binary_type = quxlang::binary::elf;
    target.module_configurations["main"].source = "main_arm32";

    bundle.targets["arm32"] = target;
    bundle.module_sources["main_arm32"].files["main.qxs"] = quxlang::source_file{.contents = with_test_language_declaration("::main VAR I32;")};

    quxlang::compiler_querygraph graph(bundle, "arm32", bundle.targets.at("arm32").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::type_symbol const context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto evaluate = [&](std::string const& keyword) -> bool
    {
        return graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
            .expr = quxlang::expression_value_keyword{.keyword = keyword},
            .context = context,
        });
    };

    EXPECT_TRUE(evaluate("UNWIND_FORMAT_IS_DWARF_EH_FRAME"));
    EXPECT_FALSE(evaluate("UNWIND_FORMAT_IS_ARM_EHABI"));
}

TEST(querygraph_queries, unwind_format_keywords_filter_include_if_declarations)
{
    auto bundle = make_single_main_source_bundle(R"QX(
::selected INCLUDE_IF(UNWIND_FORMAT_IS_DWARF_EH_FRAME) VAR I32;
::filtered INCLUDE_IF(UNWIND_FORMAT_IS_WINDOWS_SEH) VAR I32;
)QX");
    auto graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const selected = quxlang::type_symbol(quxlang::subsymbol{main, "selected"});
    quxlang::type_symbol const filtered = quxlang::type_symbol(quxlang::subsymbol{main, "filtered"});

    EXPECT_EQ(graph.make_request< quxlang::declaroids_query >(selected).size(), 1);
    EXPECT_TRUE(graph.make_request< quxlang::declaroids_query >(filtered).empty());
}

TEST(querygraph_queries, output_list_returns_configured_outputs_for_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph x64_graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());
    quxlang::compiler_querygraph arm64_graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());

    std::set< std::string > x64_outputs = x64_graph.make_request< quxlang::output_list_query >(std::monostate{});
    std::set< std::string > arm64_outputs = arm64_graph.make_request< quxlang::output_list_query >(std::monostate{});

    ASSERT_EQ(x64_outputs, (std::set< std::string >{"app", "util"}));
    ASSERT_EQ(arm64_outputs, (std::set< std::string >{"ios_app"}));
}

TEST(querygraph_queries, output_list_returns_default_when_outputs_are_omitted)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::set< std::string > outputs = graph.make_request< quxlang::output_list_query >(std::monostate{});

    ASSERT_EQ(outputs, (std::set< std::string >{"default"}));
}

TEST(querygraph_queries, output_binary_information_returns_configured_output)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::output_query_output output = graph.make_request< quxlang::output_binary_information_query >("app");

    EXPECT_EQ(output.output_name, "app");
    EXPECT_EQ(output.module_name, "main");
    ASSERT_TRUE(output.main_functanoid.has_value());
    EXPECT_EQ(*output.main_functanoid, parse_type_symbol_text("main"));
    EXPECT_EQ(output.type, quxlang::output_kind::executable);
}

TEST(querygraph_queries, output_binary_information_returns_default_output)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main VAR I32;");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::output_query_output output = graph.make_request< quxlang::output_binary_information_query >("default");

    EXPECT_EQ(output.output_name, "default");
    EXPECT_EQ(output.module_name, "main");
    ASSERT_TRUE(output.main_functanoid.has_value());
    EXPECT_EQ(*output.main_functanoid, parse_type_symbol_text("::main#()"));
    EXPECT_EQ(output.type, quxlang::output_kind::executable);
}

TEST(querygraph_queries, output_binaries_information_returns_all_outputs)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::map< std::string, quxlang::output_query_output > outputs = graph.make_request< quxlang::output_binaries_information_query >(std::monostate{});

    ASSERT_EQ(outputs.size(), static_cast< std::size_t >(2));
    ASSERT_TRUE(outputs.contains("app"));
    ASSERT_TRUE(outputs.contains("util"));
    EXPECT_EQ(outputs.at("util").module_name, "util");
    ASSERT_TRUE(outputs.at("util").main_functanoid.has_value());
    EXPECT_EQ(*outputs.at("util").main_functanoid, parse_type_symbol_text("::main#()"));
    EXPECT_EQ(outputs.at("util").type, quxlang::output_kind::shared_library);
}

TEST(querygraph_queries, output_binary_information_returns_unit_test_suite_without_main)
{
    quxlang::source_bundle bundle = make_unit_test_suite_source_bundle("::case_a UNIT_TEST { }");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::output_query_output output = graph.make_request< quxlang::output_binary_information_query >("tests");

    EXPECT_EQ(output.output_name, "tests");
    EXPECT_EQ(output.module_name, "main");
    EXPECT_FALSE(output.main_functanoid.has_value());
    EXPECT_EQ(output.type, quxlang::output_kind::unit_test_suite);
}

TEST(querygraph_queries, output_binary_information_rejects_unknown_output)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    EXPECT_THROW(graph.make_request< quxlang::output_binary_information_query >("missing"), quxlang::compilation_error);
}

TEST(querygraph_queries, target_backend_and_llvm_options_return_defaults)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    EXPECT_EQ(graph.make_request< quxlang::target_backend_query >(std::monostate{}), quxlang::backend_kind::llvm);
    EXPECT_EQ(graph.make_request< quxlang::target_llvm_backend_options_query >(std::monostate{}).mode, quxlang::backend_llvm_mode::optimize);
    EXPECT_EQ(graph.make_request< quxlang::output_llvm_backend_options_query >("app").mode, quxlang::backend_llvm_mode::optimize);
}

TEST(querygraph_queries, unimplemented_statement_default_trap_mode_emits_unimplemented_instruction)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::probe FUNCTION(): I32
{
  UNIMPLEMENTED;
  RETURN 0;
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const probe = quxlang::type_symbol(quxlang::subsymbol{main, "probe"});

    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    quxlang::vmir2::functanoid_routine3 routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
    std::string const routine_text = quxlang::vmir2::assembler(routine).to_string(routine);
    EXPECT_NE(routine_text.find("UNIMPLEMENTED"), std::string::npos);
}

TEST(querygraph_queries, unimplemented_statement_error_mode_rejects_during_codegen)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::probe FUNCTION(): I32
{
  UNIMPLEMENTED;
  RETURN 0;
}
)QX");
    bundle.targets.at("x64").unimplemented_mode = quxlang::unimplemented_mode::error;
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const probe = quxlang::type_symbol(quxlang::subsymbol{main, "probe"});

    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    EXPECT_THROW(graph.make_request< quxlang::vm_procedure3_query >(*inst), quxlang::compilation_error);
}

TEST(querygraph_queries, compilation_error_statement_rejects_during_codegen)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::probe FUNCTION(): I32
{
  COMPILATION_ERROR "intentional failure";
  RETURN 0;
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const probe = quxlang::type_symbol(quxlang::subsymbol{main, "probe"});

    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    try
    {
        (void)graph.make_request< quxlang::vm_procedure3_query >(*inst);
        FAIL() << "Expected COMPILATION_ERROR to reject codegen";
    }
    catch (quxlang::compilation_error const& error)
    {
        EXPECT_NE(std::string(error.what()).find("intentional failure"), std::string::npos);
    }
}

TEST(querygraph_queries, compilation_error_statement_skipped_by_static_if_does_not_error)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::probe FUNCTION(): I32
{
  STATIC_IF(FALSE)
  {
    COMPILATION_ERROR "skipped failure";
  }
  RETURN 0;
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::type_symbol const probe = quxlang::type_symbol(quxlang::subsymbol{main, "probe"});

    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    EXPECT_NO_THROW((void)graph.make_request< quxlang::vm_procedure3_query >(*inst));
}

TEST(querygraph_queries, panic_is_a_terminator_and_stops_dependency_reachability)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::probe FUNCTION()
{
  PANIC "expected panic";
  COMPILATION_ERROR ON_LOWER "unreachable lowering error";
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol const probe = quxlang::subsymbol{main, "probe"};
    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    quxlang::vmir2::functanoid_routine3 const routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
    bool found_panic = false;
    for (quxlang::vmir2::executable_block const& block : routine.blocks)
    {
        if (block.terminator.has_value() && block.terminator->type_is< quxlang::vmir2::panic >())
        {
            found_panic = true;
            EXPECT_EQ(block.terminator->as< quxlang::vmir2::panic >().message, "expected panic");
        }
    }
    EXPECT_TRUE(found_panic);

    quxlang::dependencies const& dependencies = graph.make_request< quxlang::direct_dependencies_query >(quxlang::direct_dependencies_input{
        .symbol = *inst,
        .set = quxlang::dependency_set::native,
    });
    EXPECT_TRUE(dependencies.runtime_dependencies.contains(quxlang::vmir_runtime_dependency::panic));
    EXPECT_FALSE(dependencies.runtime_dependencies.contains(quxlang::vmir_runtime_dependency::assert_fail));
}

TEST(querygraph_queries, panic_default_message_is_normalized_in_vmir)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::probe FUNCTION() { PANIC; }");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const probe = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{"main"},
        .name = "probe",
    };
    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = probe,
    });
    ASSERT_TRUE(inst.has_value());

    quxlang::vmir2::functanoid_routine3 const routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
    std::string const vmir = quxlang::vmir2::assembler(routine).to_string(routine);
    EXPECT_NE(vmir.find("PANIC \"PANIC statement reached\""), std::string::npos);
}

TEST(querygraph_queries, match_and_unwrap_failures_use_panic_terminators)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::failure_variant INLINE_VARIANT VALUELESS_DEFAULT [I32, U32];

::probe FUNCTION()
{
  VAR value failure_variant;
  VAR unwrapped U32 := UNWRAP value INTO U32;
  MATCH value {
    TYPE I32 { }
    TYPE U32 { }
    DEFAULT FAIL;
  }
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const probe = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{"main"},
        .name = "probe",
    };
    std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(
        quxlang::initialization_reference{.initializee = probe});
    ASSERT_TRUE(inst.has_value());

    quxlang::vmir2::functanoid_routine3 const routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
    std::set< std::string > panic_messages;
    for (quxlang::vmir2::executable_block const& block : routine.blocks)
    {
        for (quxlang::vmir2::vm_instruction const& instruction : block.instructions)
        {
            EXPECT_FALSE(instruction.type_is< quxlang::vmir2::assert_instr >());
        }
        if (block.terminator.has_value() && block.terminator->type_is< quxlang::vmir2::panic >())
        {
            panic_messages.insert(block.terminator->as< quxlang::vmir2::panic >().message);
        }
    }
    EXPECT_TRUE(panic_messages.contains("UNWRAP encountered a valueless VARIANT"));
    EXPECT_TRUE(panic_messages.contains("UNWRAP expected VARIANT alternative U32"));
    EXPECT_TRUE(panic_messages.contains("MATCH DEFAULT FAIL reached"));
}

TEST(querygraph_queries, vmir_panic_assembler_round_trips_escaped_message)
{
    std::string const source = "PANIC \"line\\n\\\"quoted\\\"\"";
    quxlang::parsers::parsing_context context = quxlang::parsers::make_unlocated_parsing_context(source);
    std::optional< quxlang::vmir2::vm_terminator > const terminator = quxlang::parsers::vmir2::try_parse_terminator(context);
    ASSERT_TRUE(terminator.has_value());
    ASSERT_TRUE(terminator->type_is< quxlang::vmir2::panic >());
    EXPECT_EQ(terminator->as< quxlang::vmir2::panic >().message, "line\n\"quoted\"");
    EXPECT_EQ(context.iter_pos, context.iter_end);

    quxlang::vmir2::functanoid_routine3 routine;
    EXPECT_EQ(quxlang::vmir2::assembler(routine).to_string(*terminator), source);
}

TEST(querygraph_queries, match_parser_bounds_header_binding_outside_parenthesized_cast)
{
    std::string const source = R"QX({
MATCH (value AS I32) AS payload {
  CASE ok AS selected WHERE selected?? { ASSERT(TRUE); }
  CASE ok AS selected OTHERWISE { ASSERT(FALSE); }
  DEFAULT FAIL;
}
})QX";
    quxlang::parsers::parsing_context context = quxlang::parsers::make_unlocated_parsing_context(source);
    quxlang::function_block const block = quxlang::parsers::parse_function_block(context);
    ASSERT_EQ(context.iter_pos, context.iter_end);
    ASSERT_EQ(block.statements.size(), 1);
    ASSERT_TRUE(quxlang::typeis< quxlang::function_match_statement >(block.statements.front()));

    quxlang::function_match_statement const& match = quxlang::as< quxlang::function_match_statement >(block.statements.front());
    ASSERT_TRUE(match.binding_name.has_value());
    EXPECT_EQ(*match.binding_name, "payload");
    EXPECT_TRUE(quxlang::typeis< quxlang::expression_typecast >(match.subject));
    ASSERT_EQ(match.arms.size(), 2);
    EXPECT_TRUE(match.arms.at(0).where_condition.has_value());
    EXPECT_TRUE(match.arms.at(1).otherwise);
    ASSERT_TRUE(match.default_clause.has_value());
    EXPECT_TRUE(match.default_clause->fail);
}

TEST(querygraph_queries, output_llvm_backend_options_falls_back_to_target_options)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    bundle.targets.at("x64").llvm_options.mode = quxlang::backend_llvm_mode::debug;
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    EXPECT_EQ(graph.make_request< quxlang::target_llvm_backend_options_query >(std::monostate{}).mode, quxlang::backend_llvm_mode::debug);
    EXPECT_EQ(graph.make_request< quxlang::output_llvm_backend_options_query >("app").mode, quxlang::backend_llvm_mode::debug);
}

TEST(querygraph_queries, output_llvm_backend_options_uses_output_override)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    bundle.targets.at("x64").llvm_options.mode = quxlang::backend_llvm_mode::debug;
    bundle.targets.at("x64").outputs->at("app").llvm_options = quxlang::backend_llvm_options{.mode = quxlang::backend_llvm_mode::optimize};
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    EXPECT_EQ(graph.make_request< quxlang::target_llvm_backend_options_query >(std::monostate{}).mode, quxlang::backend_llvm_mode::debug);
    EXPECT_EQ(graph.make_request< quxlang::output_llvm_backend_options_query >("app").mode, quxlang::backend_llvm_mode::optimize);
    EXPECT_EQ(graph.make_request< quxlang::output_llvm_backend_options_query >("util").mode, quxlang::backend_llvm_mode::debug);
}

TEST(querygraph_queries, output_llvm_input_preserves_multiple_runtime_asm_object_refs)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::main FUNCTION(): I32 { RETURN 0; }");
    bundle.targets.at("x64").module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::first VAR I32;
::second VAR I32;
::PROGRAM_START ASM_PROCEDURE X64
{
  MOVABS RAX, OFFSET OBJECT_REF(first)
  MOVABS RBX, OFFSET OBJECT_REF(second)
}
)QX")};

    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::llvm_backend::llvm_compilable_unit const unit = graph.make_request< quxlang::output_llvm_input_query >("default");
    quxlang::type_symbol const first = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
        .name = "first",
    };
    quxlang::type_symbol const second = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
        .name = "second",
    };
    quxlang::type_symbol const runtime_start = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
        .name = "PROGRAM_START",
    };
    quxlang::dependencies const& direct_dependencies = graph.make_request< quxlang::direct_dependencies_query >(
        quxlang::direct_dependencies_input{.symbol = runtime_start, .set = quxlang::dependency_set::native});

    EXPECT_TRUE(unit.object_reference_types.contains(first));
    EXPECT_TRUE(unit.object_reference_types.contains(second));
    EXPECT_EQ(direct_dependencies.global_roots, (std::set< quxlang::type_symbol >{first, second}));
}

TEST(querygraph_queries, output_llvm_input_builds_unit_test_suite_packet)
{
    quxlang::source_bundle bundle = make_unit_test_suite_source_bundle(R"QX(
::case_a UNIT_TEST
{
  ASSERT(TRUE);
}

::nested NAMESPACE {
  ::case_b UNIT_TEST
  {
  }
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::llvm_backend::llvm_compilable_unit const unit = graph.make_request< quxlang::output_llvm_input_query >("tests");

    ASSERT_TRUE(unit.whole_module_output_kind.has_value());
    EXPECT_EQ(*unit.whole_module_output_kind, quxlang::output_kind::unit_test_suite);
    ASSERT_TRUE(unit.executable_entry_symbol.has_value());
    quxlang::type_symbol const runtime_start = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{.module_name = "RUNTIME"},
        .name = "UNIT_TESTING_PROGRAM_START",
    };
    EXPECT_EQ(*unit.executable_entry_symbol, quxlang::to_string(runtime_start));

    quxlang::type_symbol const case_a = parse_type_symbol_text("MODULE(main)::case_a");
    quxlang::type_symbol const case_b = parse_type_symbol_text("MODULE(main)::nested::case_b");
    ASSERT_EQ(unit.unit_tests.size(), static_cast< std::size_t >(2));
    EXPECT_EQ(unit.unit_tests.at(0).name, quxlang::to_string(case_a));
    EXPECT_EQ(unit.unit_tests.at(0).procedure_symbol, case_a);
    EXPECT_EQ(unit.unit_tests.at(1).name, quxlang::to_string(case_b));
    EXPECT_EQ(unit.unit_tests.at(1).procedure_symbol, case_b);
    EXPECT_TRUE(unit.inlinable_functions.contains(case_a));
    EXPECT_TRUE(unit.inlinable_functions.contains(case_b));
    EXPECT_FALSE(unit.object_reference_types.contains(quxlang::builtin_symbol{.name = "MAIN_FUNCTION"}));

    quxlang::type_symbol const count_object = quxlang::builtin_symbol{.name = "UNIT_TEST_COUNT"};
    quxlang::type_symbol const names_object = quxlang::builtin_symbol{.name = "UNIT_TEST_NAMES"};
    quxlang::type_symbol const proc_object = quxlang::builtin_symbol{.name = "UNIT_TEST_PROC"};
    ASSERT_TRUE(unit.object_reference_types.contains(count_object));
    ASSERT_TRUE(unit.object_reference_types.contains(names_object));
    ASSERT_TRUE(unit.object_reference_types.contains(proc_object));
    EXPECT_EQ(unit.object_reference_types.at(count_object), quxlang::llvm_backend::unit_test_count_object_type());
    EXPECT_EQ(unit.object_reference_types.at(names_object), quxlang::llvm_backend::unit_test_names_object_type());
    EXPECT_EQ(unit.object_reference_types.at(proc_object), quxlang::llvm_backend::unit_test_proc_object_type());
}

TEST(querygraph_queries, asm_callable_flagset_parameter_lowers_by_value)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::flags FLAGSET BITS(32) [read = 1, write = 2];

::raw ASM_PROCEDURE X64
  CALLABLE CALLCONV CCALL(@flags flags; RETURN I32)
{
  MOV RAX, RDI
  RET
}

::main FUNCTION(): I32
{
  VAR value flags := flags::read;
  RETURN raw(@flags value);
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");

    std::size_t const declaration_pos = llvm_ir.find("declare i32 @\"MODULE(main)::raw");
    ASSERT_NE(declaration_pos, std::string::npos);
    std::string const declaration_window = llvm_ir.substr(declaration_pos, 180);
    EXPECT_NE(declaration_window.find("(i32)"), std::string::npos);
    EXPECT_EQ(declaration_window.find("(ptr)"), std::string::npos);

    std::size_t const call_pos = llvm_ir.find("call i32 @\"MODULE(main)::raw");
    ASSERT_NE(call_pos, std::string::npos);
    std::string const call_window = llvm_ir.substr(call_pos, 180);
    EXPECT_NE(call_window.find("(i32 "), std::string::npos);
    EXPECT_EQ(call_window.find("(ptr "), std::string::npos);
}

TEST(querygraph_queries, parse_file_rejects_runtime_declared_symbols_in_non_runtime_module)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    EXPECT_THROW(graph.make_request< quxlang::module_ast_query >("main"), std::logic_error);
}

TEST(querygraph_queries, output_llvm_input_initializes_one_runtime_assert_fail_functanoid)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::main FUNCTION(): I32
{
  ASSERT(FALSE);
  ASSERT(FALSE, "runtime tag");
  RETURN 0;
}
)QX");
    bundle.targets.at("x64").module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::ASSERT_FAIL FUNCTION(@expr STRING_CONSTANT, @file SZ, @line SZ, @column SZ, @tag CONST -> STRING_CONSTANT)
{
}

::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX")};

    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::llvm_backend::llvm_compilable_unit const unit = graph.make_request< quxlang::output_llvm_input_query >("default");
    quxlang::llvm_backend::runtime_procedure_reference const assert_fail_ref{
        .procedure = quxlang::llvm_backend::runtime_procedure::assert_fail,
    };

    ASSERT_EQ(unit.runtime_procedures.size(), 1);
    ASSERT_TRUE(unit.runtime_procedures.contains(assert_fail_ref));
    quxlang::type_symbol const& assert_fail_symbol = unit.runtime_procedures.at(assert_fail_ref);
    EXPECT_TRUE(assert_fail_symbol.type_is< quxlang::instanciation_reference >());
    EXPECT_TRUE(unit.inlinable_functions.contains(assert_fail_symbol));
    EXPECT_TRUE(unit.procedure_linksymbols.contains(assert_fail_symbol));
    EXPECT_EQ(unit.procedure_linksymbols.at(assert_fail_symbol), quxlang::to_string(assert_fail_symbol));
}

TEST(querygraph_queries, output_llvm_input_initializes_runtime_panic_functanoid)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::main FUNCTION(): I32
{
  IF (FALSE)
  {
    PANIC "expected native panic";
  }
  RETURN 0;
}
)QX");
    bundle.targets.at("x64").module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::PANIC FUNCTION(@message STRING_CONSTANT, @file SZ, @line SZ, @column SZ)
{
}

::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX")};

    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::llvm_backend::llvm_compilable_unit const unit = graph.make_request< quxlang::output_llvm_input_query >("default");
    quxlang::llvm_backend::runtime_procedure_reference const panic_ref{
        .procedure = quxlang::llvm_backend::runtime_procedure::panic,
    };

    ASSERT_EQ(unit.runtime_procedures.size(), 1);
    ASSERT_TRUE(unit.runtime_procedures.contains(panic_ref));
    quxlang::type_symbol const& panic_symbol = unit.runtime_procedures.at(panic_ref);
    EXPECT_TRUE(panic_symbol.type_is< quxlang::instanciation_reference >());
    EXPECT_TRUE(unit.inlinable_functions.contains(panic_symbol));
    EXPECT_TRUE(unit.procedure_linksymbols.contains(panic_symbol));

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");
    EXPECT_NE(llvm_ir.find("call void @\"MODULE(RUNTIME)::PANIC"), std::string::npos);
    EXPECT_NE(llvm_ir.find("expected native panic"), std::string::npos);
}

TEST(querygraph_queries, native_panic_without_runtime_reports_targeted_diagnostic)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::main FUNCTION(): I32
{
  PANIC "missing runtime";
}
)QX");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    try
    {
        (void)graph.make_request< quxlang::output_llvm_input_query >("default");
        FAIL() << "Expected native PANIC lowering to require the runtime PANIC procedure";
    }
    catch (quxlang::compilation_error const& error)
    {
        EXPECT_NE(std::string(error.what()).find("Native PANIC lowering requires MODULE(RUNTIME)::PANIC"), std::string::npos);
    }
}

TEST(querygraph_queries, match_tablebranch_lowers_to_llvm_switch)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::switch_variant INLINE_VARIANT [I32 DEFAULT, VOID];

::main FUNCTION(): I32
{
  VAR value switch_variant;
  VAR result I32 := 0;
  MATCH value AS payload {
    TYPE I32 { result := payload; }
    TYPE VOID { result := 1; }
  }
  RETURN result;
}
)QX");
    bundle.targets.at("x64").module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::PANIC FUNCTION(@message STRING_CONSTANT, @file SZ, @line SZ, @column SZ)
{
}

::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX")};
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");
    EXPECT_NE(llvm_ir.find("switch i8"), std::string::npos);
}

TEST(querygraph_queries, output_llvm_input_initializes_initguard_runtime_functanoids)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::custom_global VAR custom_record;

::main FUNCTION(): I32
{
  RETURN custom_global.x;
}
)QX");
    bundle.targets.at("x64").module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::INITGUARD_TRY_ACQUIRE FUNCTION(@guard MUT& INITGUARD): BOOL
{
  RETURN TRUE;
}

::INITGUARD_COMPLETE FUNCTION(@guard MUT& INITGUARD)
{
}

::INITGUARD_ABORT FUNCTION(@guard MUT& INITGUARD)
{
}

::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX")};

    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::llvm_backend::llvm_compilable_unit const unit = graph.make_request< quxlang::output_llvm_input_query >("default");

    quxlang::llvm_backend::runtime_procedure_reference const try_acquire_ref{
        .procedure = quxlang::llvm_backend::runtime_procedure::initguard_try_acquire,
    };
    quxlang::llvm_backend::runtime_procedure_reference const complete_ref{
        .procedure = quxlang::llvm_backend::runtime_procedure::initguard_complete,
    };

    ASSERT_TRUE(unit.runtime_procedures.contains(try_acquire_ref));
    ASSERT_TRUE(unit.runtime_procedures.contains(complete_ref));
    EXPECT_TRUE(unit.runtime_procedures.at(try_acquire_ref).type_is< quxlang::instanciation_reference >());
    EXPECT_TRUE(unit.runtime_procedures.at(complete_ref).type_is< quxlang::instanciation_reference >());
    EXPECT_TRUE(unit.inlinable_functions.contains(unit.runtime_procedures.at(try_acquire_ref)));
    EXPECT_TRUE(unit.inlinable_functions.contains(unit.runtime_procedures.at(complete_ref)));
}

TEST(querygraph_queries, runtime_initguard_implementation_uses_atomic_busy_loop)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"QX(
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::custom_global VAR custom_record;

::main FUNCTION(): I32
{
  RETURN custom_global.x;
}
)QX");
    quxlang::target_configuration& target = bundle.targets.at("x64");
    target.outputs = std::map< std::string, quxlang::output_config >{
        {"default", quxlang::output_config{.type = quxlang::output_kind::executable, .module = "main", .main_functanoid = "main"}},
    };
    target.module_configurations["RUNTIME"].source = "runtime_x64";
    bundle.module_sources["runtime_x64"].files["runtime.qxs"] = quxlang::source_file{.contents = with_test_language_declaration(R"QX(
::ASSERT_FAIL FUNCTION(@expr STRING_CONSTANT, @file SZ, @line SZ, @column SZ, @tag CONST -> STRING_CONSTANT)
{
}

::INITGUARD_TRY_ACQUIRE FUNCTION(@guard MUT& INITGUARD): BOOL
{
  WHILE (TRUE)
  {
    VAR state UINTPTR := guard.LOAD#ATOMIC_ACQUIRE();
    IF (state == 2)
    {
      RETURN FALSE;
    }
    IF (state == 0)
    {
      VAR expected UINTPTR := 0;
      IF (guard.COMPARE_EXCHANGE#(@SUCCESS ATOMIC_ACQREL, @FAILURE ATOMIC_ACQUIRE)(expected, 1))
      {
        RETURN TRUE;
      }
    }
    ELSE IF (state != 1)
    {
      ASSERT(FALSE, "Invalid initguard state");
    }
  }
}

::INITGUARD_COMPLETE FUNCTION(@guard MUT& INITGUARD)
{
  guard.STORE#ATOMIC_RELEASE(2);
}

::INITGUARD_ABORT FUNCTION(@guard MUT& INITGUARD)
{
  guard.STORE#ATOMIC_RELEASE(0);
}

::PROGRAM_START ASM_PROCEDURE X64
{
  RET
}
)QX")};

    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");

    EXPECT_NE(llvm_ir.find("load atomic i64"), std::string::npos);
    EXPECT_NE(llvm_ir.find("cmpxchg ptr"), std::string::npos);
    EXPECT_GE(count_substrings(llvm_ir, "store atomic i64"), 1U);
    EXPECT_NE(llvm_ir.find(" release,"), std::string::npos);
    EXPECT_EQ(llvm_ir.find("pthread"), std::string::npos);
    EXPECT_EQ(llvm_ir.find("futex"), std::string::npos);
    EXPECT_EQ(llvm_ir.find("WaitOnAddress"), std::string::npos);
}

TEST(querygraph_queries, llvm_gentest_atomic_operations_generate_valid_llvm_ir)
{
    std::filesystem::path testdata = QUXLANG_TESTS_TESTDDATA_PATH;
    quxlang::source_bundle sources = quxlang::load_bundle_sources_for_targets(testdata / "llvm_gentest", {});
    quxlang::compiler_querygraph graph(sources, "linux-x64", sources.targets.at("linux-x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");

    EXPECT_NE(llvm_ir.find("store atomic i32"), std::string::npos);
    EXPECT_NE(llvm_ir.find("load atomic i32"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw add ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw sub ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw and ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw or ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw xor ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("cmpxchg ptr"), std::string::npos);
    EXPECT_NE(llvm_ir.find("atomicrmw.loop"), std::string::npos);
    EXPECT_EQ(llvm_ir.find("store atomic i24"), std::string::npos);
    EXPECT_EQ(llvm_ir.find("load atomic i24"), std::string::npos);

    for (std::string const prefix : {"atomicrmw ", "cmpxchg ptr"})
    {
        std::size_t position = llvm_ir.find(prefix);
        while (position != std::string::npos)
        {
            std::size_t const line_end = llvm_ir.find('\n', position);
            std::string const line = llvm_ir.substr(position, line_end - position);
            EXPECT_EQ(line.find("i24"), std::string::npos) << line;
            position = llvm_ir.find(prefix, line_end);
        }
    }
}

TEST(querygraph_queries, numeric_literal_prefers_unsigned_integer_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::type_symbol numeric_literal = quxlang::numeric_literal_type{.value = "42"};
    quxlang::type_symbol unsigned_integer = quxlang::int_type{.bits = 64, .has_sign = false};
    quxlang::type_symbol signed_integer = quxlang::int_type{.bits = 64, .has_sign = true};

    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = numeric_literal,
        .better_to = unsigned_integer,
        .worse_to = signed_integer,
    }));
    ASSERT_FALSE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = numeric_literal,
        .better_to = signed_integer,
        .worse_to = unsigned_integer,
    }));
}

TEST(querygraph_queries, numeric_literal_out_of_range_narrowing_has_no_adaptation)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    // 4294967291 does not fit in I32 (signed 32-bit), so adaptation rank should be nullopt.
    quxlang::type_symbol literal = quxlang::numeric_literal_type{.value = "4294967291"};
    quxlang::type_symbol i32_type = quxlang::int_type{.bits = 32, .has_sign = true};

    auto rank = graph.make_request< quxlang::argument_adaptation_rank_query >(quxlang::argument_init_input{
        .from = literal,
        .to = i32_type,
    });
    ASSERT_FALSE(rank.has_value());
}

TEST(querygraph_queries, numeric_literal_in_range_u32_has_adaptation)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    // 4294967291 fits in U32 (unsigned 32-bit), so adaptation rank should be present.
    quxlang::type_symbol literal = quxlang::numeric_literal_type{.value = "4294967291"};
    quxlang::type_symbol u32_type = quxlang::int_type{.bits = 32, .has_sign = false};

    auto rank = graph.make_request< quxlang::argument_adaptation_rank_query >(quxlang::argument_init_input{
        .from = literal,
        .to = u32_type,
    });
    ASSERT_TRUE(rank.has_value());
}

TEST(querygraph_queries, numeric_literal_to_numeric_constant_adaptation)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::type_symbol literal = quxlang::numeric_literal_type{.value = "123"};
    quxlang::type_symbol numeric_constant = quxlang::readonly_constant{.kind = quxlang::constant_kind::numeric};

    auto rank = graph.make_request< quxlang::argument_adaptation_rank_query >(quxlang::argument_init_input{
        .from = literal,
        .to = numeric_constant,
    });
    ASSERT_TRUE(rank.has_value());
}

TEST(querygraph_queries, temporary_source_prefers_temporary_reference_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);

    quxlang::type_symbol byte_type = quxlang::byte_type{};
    quxlang::type_symbol temporary_byte_ref = quxlang::make_tref(byte_type);
    quxlang::type_symbol const_byte_ref = quxlang::make_cref(byte_type);

    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = temporary_byte_ref,
        .worse_to = const_byte_ref,
    }));
    ASSERT_FALSE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = const_byte_ref,
        .worse_to = temporary_byte_ref,
    }));
    ASSERT_TRUE(graph.make_request< quxlang::argument_adaptation_is_better_fit_query >(quxlang::argument_adaptation_better_fit_input{
        .from = temporary_byte_ref,
        .better_to = temporary_byte_ref,
        .worse_to = const_byte_ref,
        .adaptations = quxlang::allowed_adaptations::none,
    }));
}

TEST(querygraph_queries, module_source_name_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph x64_graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                           quxlang::tests::current_test_graph_dump_path());

    quxlang::compiler_querygraph arm64_graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                             quxlang::tests::current_test_graph_dump_path());

    ASSERT_EQ(x64_graph.make_request< quxlang::module_source_name_query >("main"), "main_x64");
    ASSERT_EQ(arm64_graph.make_request< quxlang::module_source_name_query >("main"), "main_arm64");
}

TEST(querygraph_queries, module_source_name_map_uses_configured_target)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "arm64", bundle.targets.at("arm64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::module_source_name_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at("main"), "main_arm64");
    ASSERT_EQ(resolved.at("util"), "util_shared");
}

TEST(querygraph_queries, module_options_map_uses_configured_target)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    auto graph = make_x64_graph(bundle);
    auto answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});

    auto resolved = graph.make_request< quxlang::module_options_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at(answer), "7");
}

TEST(querygraph_queries, module_options_map_resolves_overloaded_option_symbols)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::holder STRUCT { .answer OPTION NUMBER DEFAULT_VALUE(5); }");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    bundle.targets.at("x64").module_configurations.at("main").option_values["holder::.answer"] = "9";
    auto graph = make_x64_graph(bundle);
    auto module_answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});
    auto holder = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "holder"});
    auto holder_answer = quxlang::type_symbol(quxlang::submember{holder, "answer"});

    auto resolved = graph.make_request< quxlang::module_options_map_query >(std::monostate{});

    ASSERT_EQ(resolved.at(module_answer), "7");
    ASSERT_EQ(resolved.at(holder_answer), "9");
}

TEST(querygraph_queries, module_sources_returns_source_files_for_logical_module)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::module_sources_query >("util");

    ASSERT_TRUE(resolved.files.contains("util.qxs"));
    ASSERT_EQ(resolved.files.at("util.qxs").get().contents, with_test_language_declaration("::util VAR I32;"));
}

TEST(querygraph_queries, source_file_index_assigns_deterministic_global_ids)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto resolved = graph.make_request< quxlang::source_file_index_query >(std::monostate{});

    ASSERT_EQ(resolved.file_to_id.size(), 3);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qxs"}), 0);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qxs"}), 1);
    ASSERT_EQ(resolved.file_to_id.at(quxlang::source_file_name{.source_module = "util_shared", .relative_path = "util.qxs"}), 2);
}

TEST(querygraph_queries, source_file_id_is_unique_across_modules)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());

    auto arm_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_arm64", .relative_path = "main.qxs"});
    auto x64_id = graph.make_request< quxlang::source_file_id_query >(quxlang::source_file_name{.source_module = "main_x64", .relative_path = "main.qxs"});

    ASSERT_NE(arm_id, x64_id);
}

TEST(querygraph_queries, source_file_name_round_trips_from_id)
{
    quxlang::source_bundle bundle = make_test_source_bundle();
    quxlang::compiler_querygraph graph(bundle, "x64", bundle.targets.at("x64").target_output_config,
                                       quxlang::tests::current_test_graph_dump_path());
    quxlang::source_file_name input{.source_module = "util_shared", .relative_path = "util.qxs"};

    auto id = graph.make_request< quxlang::source_file_id_query >(input);
    auto output = graph.make_request< quxlang::source_file_name_query >(id);

    ASSERT_EQ(output, input);
}

TEST(querygraph_queries, option_declaration_resolves_as_option_symbol)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    auto graph = make_x64_graph(bundle);
    auto answer = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "answer"});

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(answer), quxlang::symbol_kind::option);
}

TEST(querygraph_queries, test_declaration_modes_drive_static_and_unit_discovery)
{
    auto bundle = make_single_main_source_bundle(R"(
::static_case STATIC_TEST { }
::unit_case UNIT_TEST { }
::dual_case DUAL_TEST { }
)");
    auto graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol const static_case = quxlang::subsymbol{main, "static_case"};
    quxlang::type_symbol const unit_case = quxlang::subsymbol{main, "unit_case"};
    quxlang::type_symbol const dual_case = quxlang::subsymbol{main, "dual_case"};

    std::set< quxlang::type_symbol > const static_tests = graph.make_request< quxlang::list_static_tests_query >(main);
    std::set< quxlang::type_symbol > const unit_tests = graph.make_request< quxlang::list_unit_tests_query >(main);

    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(static_case), quxlang::symbol_kind::test);
    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(unit_case), quxlang::symbol_kind::test);
    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(dual_case), quxlang::symbol_kind::test);

    EXPECT_TRUE(graph.make_request< quxlang::test_is_enabled_for_static_testing_query >(static_case));
    EXPECT_FALSE(graph.make_request< quxlang::test_is_enabled_for_unit_testing_query >(static_case));
    EXPECT_FALSE(graph.make_request< quxlang::test_is_enabled_for_static_testing_query >(unit_case));
    EXPECT_TRUE(graph.make_request< quxlang::test_is_enabled_for_unit_testing_query >(unit_case));
    EXPECT_TRUE(graph.make_request< quxlang::test_is_enabled_for_static_testing_query >(dual_case));
    EXPECT_TRUE(graph.make_request< quxlang::test_is_enabled_for_unit_testing_query >(dual_case));

    EXPECT_TRUE(static_tests.contains(static_case));
    EXPECT_FALSE(static_tests.contains(unit_case));
    EXPECT_TRUE(static_tests.contains(dual_case));
    EXPECT_FALSE(unit_tests.contains(static_case));
    EXPECT_TRUE(unit_tests.contains(unit_case));
    EXPECT_TRUE(unit_tests.contains(dual_case));
}

TEST(querygraph_queries, nested_namespace_subdeclaroids_resolve)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::outer NAMESPACE
{
  ::inner NAMESPACE
  {
    ::value VAR I32;
  }
}
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol outer = quxlang::subsymbol{main, "outer"};
    quxlang::type_symbol inner = quxlang::subsymbol{outer, "inner"};
    quxlang::type_symbol value = quxlang::subsymbol{inner, "value"};

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(value), quxlang::symbol_kind::global_variable);
}

TEST(querygraph_queries, template_subtags_resolve_type_and_value_bindings)
{
    quxlang::type_symbol const parsed_tag = parse_type_symbol_text("templ#(@x I32)$x");
    ASSERT_TRUE(quxlang::typeis< quxlang::subtag_type >(parsed_tag));
    quxlang::subtag_type const& parsed_subtag = quxlang::as< quxlang::subtag_type >(parsed_tag);
    ASSERT_EQ(parsed_subtag.name, "x");
    ASSERT_TRUE(quxlang::typeis< quxlang::initialization_reference >(parsed_subtag.of));

    std::string const printed_tag = quxlang::to_string(parsed_tag);
    ASSERT_NE(printed_tag.find("$x"), std::string::npos);
    ASSERT_EQ(parse_type_symbol_text(printed_tag), parsed_tag);

    auto bundle = make_single_main_source_bundle(R"(
::local_count VAR U64;

::alias_holder TEMPLATE(@T TYPE AUTO(t), @count:local_count VALUE U64, @public_count VALUE U64) STRUCT
{
  .value VAR t;
}

::shadow_holder TEMPLATE(@T TYPE AUTO(t)) STRUCT
{
  ::t VAR I64;
}
)");
    auto graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol const i32 = quxlang::int_type{32, true};
    quxlang::type_symbol const u64 = quxlang::int_type{64, false};
    quxlang::type_symbol const alias_input = parse_type_symbol_text("MODULE(main)::alias_holder#(@T I32, @count 7, @public_count 9)");

    std::optional< quxlang::type_symbol > alias_context = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = alias_input,
    });
    ASSERT_TRUE(alias_context.has_value());

    std::optional< quxlang::type_symbol > type_alias = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = parse_type_symbol_text("MODULE(main)::alias_holder#(@T I32, @count 7, @public_count 9)$t"),
    });
    ASSERT_EQ(type_alias, std::optional< quxlang::type_symbol >(i32));

    std::optional< quxlang::type_symbol > local_value_tag = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = parse_type_symbol_text("MODULE(main)::alias_holder#(@T I32, @count 7, @public_count 9)$local_count"),
    });
    ASSERT_TRUE(local_value_tag.has_value());
    ASSERT_TRUE(quxlang::typeis< quxlang::subtag_type >(*local_value_tag));
    ASSERT_EQ(quxlang::as< quxlang::subtag_type >(*local_value_tag).name, "local_count");
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(*local_value_tag), quxlang::symbol_kind::global_variable);
    ASSERT_EQ(graph.make_request< quxlang::variable_type_query >(*local_value_tag), u64);
    ASSERT_TRUE(graph.make_request< quxlang::global_is_antestatal_static_query >(*local_value_tag));
    ASSERT_TRUE(quxlang::typeis< std::monostate >(graph.make_request< quxlang::symboid_query >(*local_value_tag)));

    std::optional< quxlang::type_symbol > api_value_tag = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = parse_type_symbol_text("MODULE(main)::alias_holder#(@T I32, @count 7, @public_count 9)$count"),
    });
    ASSERT_FALSE(api_value_tag.has_value());

    quxlang::antestatal_value const local_value = graph.make_request< quxlang::antestatal_static_value_query >(*local_value_tag);
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(local_value));
    std::vector< std::byte > const& local_bytes = quxlang::as< quxlang::antestatal_primitive >(local_value).value;
    ASSERT_GE(local_bytes.size(), static_cast< std::size_t >(1));
    ASSERT_EQ(std::to_integer< std::uint32_t >(local_bytes.at(0)), 7);

    std::uint64_t const constexpr_read = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{*local_value_tag},
        .context = main,
    });
    ASSERT_EQ(constexpr_read, 7);

    std::optional< quxlang::type_symbol > public_value_tag = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = parse_type_symbol_text("MODULE(main)::alias_holder#(@T I32, @count 7, @public_count 9)$public_count"),
    });
    ASSERT_TRUE(public_value_tag.has_value());
    ASSERT_TRUE(quxlang::typeis< quxlang::subtag_type >(*public_value_tag));
    ASSERT_EQ(quxlang::as< quxlang::subtag_type >(*public_value_tag).name, "public_count");
    quxlang::antestatal_value const public_value = graph.make_request< quxlang::antestatal_static_value_query >(*public_value_tag);
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(public_value));
    std::vector< std::byte > const& public_bytes = quxlang::as< quxlang::antestatal_primitive >(public_value).value;
    ASSERT_GE(public_bytes.size(), static_cast< std::size_t >(1));
    ASSERT_EQ(std::to_integer< std::uint32_t >(public_bytes.at(0)), 9);

    std::optional< quxlang::type_symbol > free_local = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = *alias_context,
        .type = quxlang::freebound_identifier{"local_count"},
    });
    ASSERT_EQ(free_local, local_value_tag);

    std::optional< quxlang::type_symbol > decltype_local = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = *alias_context,
        .type = parse_type_symbol_text("DECLTYPE(local_count)"),
    });
    ASSERT_EQ(decltype_local, std::optional< quxlang::type_symbol >(u64));

    std::optional< quxlang::type_symbol > shadow_context = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = parse_type_symbol_text("MODULE(main)::shadow_holder#(@T I32)"),
    });
    ASSERT_TRUE(shadow_context.has_value());

    std::optional< quxlang::type_symbol > shadow_lookup = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = *shadow_context,
        .type = quxlang::freebound_identifier{"t"},
    });
    ASSERT_EQ(shadow_lookup, std::optional< quxlang::type_symbol >(quxlang::subsymbol{*shadow_context, "t"}));
}

TEST(querygraph_queries, enum_info_normalizes_values_defaults_and_reservations)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::choice ENUM BITS(8) [none = NULL, x, RESERVED FROM(2) TO(3), y = 4] ALLOW_UNKNOWN {
    ::zero STATIC choice := none;
}
::ipc_choice IPC_ENUM BITS(8) [zero = 0, one = 1];
::record STRUCT { .value VAR I32; }
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol choice = quxlang::subsymbol{main, "choice"};
    quxlang::type_symbol ipc_choice = quxlang::subsymbol{main, "ipc_choice"};
    quxlang::type_symbol record = quxlang::subsymbol{main, "record"};
    quxlang::type_symbol none = quxlang::subsymbol{choice, "none"};
    quxlang::type_symbol zero = quxlang::subsymbol{choice, "zero"};

    quxlang::enum_info info = graph.make_request< quxlang::enum_info_query >(choice);
    quxlang::enum_info ipc_info = graph.make_request< quxlang::enum_info_query >(ipc_choice);

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(choice), quxlang::symbol_kind::class_);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(ipc_choice), quxlang::symbol_kind::class_);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(choice), quxlang::class_kind::enum_);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(ipc_choice), quxlang::class_kind::enum_);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(record), quxlang::symbol_kind::class_);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(record), quxlang::class_kind::struct_);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(quxlang::int_type{.bits = 32, .has_sign = true}), quxlang::class_kind::primitive);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(quxlang::array_type{.element_type = quxlang::int_type{.bits = 32, .has_sign = true}, .element_count = quxlang::expression_numeric_literal{.value = "4"}}), quxlang::class_kind::primitive);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(none), quxlang::symbol_kind::enum_value);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(zero), quxlang::symbol_kind::global_variable);
    EXPECT_EQ(info.bits, 8);
    EXPECT_EQ(info.storage_bytes, 1);
    EXPECT_TRUE(info.allow_unknown);
    EXPECT_FALSE(info.is_ipc);
    EXPECT_EQ(ipc_info.bits, 8);
    EXPECT_EQ(ipc_info.storage_bytes, 1);
    EXPECT_TRUE(ipc_info.is_ipc);
    ASSERT_EQ(info.values.size(), 3);
    ASSERT_EQ(info.reserved_ranges.size(), 1);
    EXPECT_EQ(info.reserved_ranges.at(0).from, 2);
    EXPECT_EQ(info.reserved_ranges.at(0).to, 3);
    ASSERT_TRUE(info.null_value_name.has_value());
    EXPECT_EQ(*info.null_value_name, "none");
    ASSERT_TRUE(info.default_value_name.has_value());
    EXPECT_EQ(*info.default_value_name, "none");

    std::map< std::string, quxlang::enum_value_info > values;
    for (quxlang::enum_value_info const& value : info.values)
    {
        values[value.name] = value;
    }

    EXPECT_EQ(values.at("none").value, 0);
    EXPECT_TRUE(values.at("none").is_null);
    EXPECT_TRUE(values.at("none").is_default);
    EXPECT_EQ(values.at("x").value, 1);
    EXPECT_FALSE(values.at("x").is_explicit);
    EXPECT_EQ(values.at("y").value, 4);
    EXPECT_TRUE(values.at("y").is_explicit);

    quxlang::class_placement_info placement = graph.make_request< quxlang::class_placement_info_query >(choice);
    EXPECT_EQ(placement.size, 1);
    EXPECT_EQ(placement.alignment, 1);
}

TEST(querygraph_queries, fusion_info_and_layout_normalize_all_declaration_forms)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::boxed UNION NO_DEFAULT_COPY {
    .none OPTION DEFAULT VOID;
    ::helper FUNCTION() {}
    .number OPTION I32;
}
::inline_union INLINE_UNION NEVER_VALUELESS {
    .small OPTION U8;
    .wide OPTION U64;
}
::boxed_variant VARIANT VALUELESS_DEFAULT [I32, VOID];
::inline_variant INLINE_VARIANT [U8, U64 DEFAULT] {
    ::helper FUNCTION() {}
}
::inline_voids INLINE_VARIANT [VOID];
::generic TEMPLATE(@T TYPE AUTO(t)) INLINE_VARIANT [t, VOID];
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol const main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol const boxed = quxlang::subsymbol{main, "boxed"};
    quxlang::type_symbol const inline_union = quxlang::subsymbol{main, "inline_union"};
    quxlang::type_symbol const boxed_variant = quxlang::subsymbol{main, "boxed_variant"};
    quxlang::type_symbol const inline_variant = quxlang::subsymbol{main, "inline_variant"};
    quxlang::type_symbol const inline_voids = quxlang::subsymbol{main, "inline_voids"};

    quxlang::union_info const boxed_info = graph.make_request< quxlang::union_info_query >(boxed);
    ASSERT_EQ(boxed_info.options.size(), 2);
    EXPECT_EQ(boxed_info.options.at(0).name, "none");
    EXPECT_TRUE(quxlang::typeis< quxlang::void_type >(boxed_info.options.at(0).type));
    EXPECT_EQ(boxed_info.options.at(1).name, "number");
    EXPECT_EQ(boxed_info.options.at(1).type, quxlang::type_symbol(quxlang::int_type{.bits = 32, .has_sign = true}));
    ASSERT_TRUE(boxed_info.properties.default_index.has_value());
    EXPECT_EQ(*boxed_info.properties.default_index, 0);
    EXPECT_FALSE(boxed_info.properties.is_inline);
    EXPECT_FALSE(boxed_info.properties.generate_copy);
    EXPECT_EQ(graph.make_request< quxlang::class_type_query >(boxed), quxlang::class_kind::union_);
    EXPECT_EQ(graph.make_request< quxlang::symboid_subdeclaroids_query >(boxed).size(), 1);

    quxlang::variant_info const boxed_variant_info = graph.make_request< quxlang::variant_info_query >(boxed_variant);
    ASSERT_EQ(boxed_variant_info.alternatives.size(), 2);
    EXPECT_TRUE(boxed_variant_info.properties.valueless_default);
    EXPECT_FALSE(boxed_variant_info.properties.default_index.has_value());
    EXPECT_EQ(graph.make_request< quxlang::class_type_query >(boxed_variant), quxlang::class_kind::variant);

    quxlang::fusion_layout const boxed_layout = graph.make_request< quxlang::fusion_layout_query >(boxed);
    EXPECT_FALSE(boxed_layout.is_inline);
    EXPECT_EQ(boxed_layout.placement.size, 16);
    EXPECT_EQ(boxed_layout.placement.alignment, 8);
    EXPECT_EQ(boxed_layout.payload_offset, 0);
    EXPECT_EQ(boxed_layout.tag_offset, 8);
    EXPECT_EQ(boxed_layout.tag_type, quxlang::type_symbol(quxlang::int_type{.bits = 64, .has_sign = false}));
    ASSERT_TRUE(boxed_layout.valueless_tag.has_value());
    EXPECT_EQ(*boxed_layout.valueless_tag, 2);
    EXPECT_EQ(graph.make_request< quxlang::class_placement_info_query >(boxed), boxed_layout.placement);

    quxlang::fusion_layout const inline_union_layout = graph.make_request< quxlang::fusion_layout_query >(inline_union);
    EXPECT_TRUE(inline_union_layout.is_inline);
    EXPECT_EQ(inline_union_layout.payload_placement.size, 8);
    EXPECT_EQ(inline_union_layout.payload_placement.alignment, 8);
    EXPECT_EQ(inline_union_layout.tag_offset, 8);
    EXPECT_EQ(inline_union_layout.tag_type, quxlang::type_symbol(quxlang::int_type{.bits = 8, .has_sign = false}));
    EXPECT_EQ(inline_union_layout.placement.size, 16);
    EXPECT_EQ(inline_union_layout.placement.alignment, 8);
    EXPECT_FALSE(inline_union_layout.valueless_tag.has_value());

    quxlang::variant_info const inline_variant_info = graph.make_request< quxlang::variant_info_query >(inline_variant);
    ASSERT_TRUE(inline_variant_info.properties.default_index.has_value());
    EXPECT_EQ(*inline_variant_info.properties.default_index, 1);
    quxlang::fusion_layout const inline_variant_layout = graph.make_request< quxlang::fusion_layout_query >(inline_variant);
    EXPECT_TRUE(inline_variant_layout.is_inline);
    ASSERT_TRUE(inline_variant_layout.valueless_tag.has_value());
    EXPECT_EQ(*inline_variant_layout.valueless_tag, 2);
    EXPECT_EQ(graph.make_request< quxlang::symboid_subdeclaroids_query >(inline_variant).size(), 1);

    quxlang::fusion_layout const inline_voids_layout = graph.make_request< quxlang::fusion_layout_query >(inline_voids);
    EXPECT_EQ(inline_voids_layout.payload_placement.size, 0);
    EXPECT_EQ(inline_voids_layout.payload_placement.alignment, 1);
    EXPECT_EQ(inline_voids_layout.tag_offset, 0);
    EXPECT_EQ(inline_voids_layout.placement.size, 1);
    EXPECT_EQ(inline_voids_layout.placement.alignment, 1);

    quxlang::type_symbol const generic_input = parse_type_symbol_text("MODULE(main)::generic#(@T I32)");
    std::optional< quxlang::type_symbol > const generic = graph.make_request< quxlang::lookup_query >(quxlang::contextual_type_reference{
        .context = main,
        .type = generic_input,
    });
    ASSERT_TRUE(generic.has_value());
    quxlang::variant_info const generic_info = graph.make_request< quxlang::variant_info_query >(*generic);
    ASSERT_EQ(generic_info.alternatives.size(), 2);
    EXPECT_EQ(generic_info.alternatives.at(0), quxlang::type_symbol(quxlang::int_type{.bits = 32, .has_sign = true}));
    EXPECT_TRUE(quxlang::typeis< quxlang::void_type >(generic_info.alternatives.at(1)));
}

TEST(querygraph_queries, invalid_fusion_and_match_fixtures_report_expected_diagnostics)
{
    /// Query boundary that is expected to reject an invalid source fixture.
    enum class invalid_fixture_query
    {
        module_ast,
        union_info,
        variant_info,
        vm_routine,
    };

    /// Structured compilation diagnostic expected from an invalid source fixture.
    enum class invalid_fixture_diagnostic
    {
        syntax,
        semantic,
    };

    /// One standalone invalid Quxlang source and its expected rejection boundary.
    struct invalid_fixture
    {
        std::filesystem::path relative_path;
        invalid_fixture_query query;
        invalid_fixture_diagnostic diagnostic;
        std::string expected_message;
    };

    std::vector< invalid_fixture > const fixtures{
        {"fusion/empty_union.qxs", invalid_fixture_query::union_info, invalid_fixture_diagnostic::semantic, "UNION must declare at least one option"},
        {"fusion/duplicate_union_option.qxs", invalid_fixture_query::union_info, invalid_fixture_diagnostic::semantic, "Duplicate UNION option name"},
        {"fusion/conflicting_valueless_union.qxs", invalid_fixture_query::union_info, invalid_fixture_diagnostic::semantic,
         "NEVER_VALUELESS and VALUELESS_DEFAULT cannot be combined"},
        {"fusion/multiple_union_defaults.qxs", invalid_fixture_query::union_info, invalid_fixture_diagnostic::semantic,
         "UNION declares more than one DEFAULT option"},
        {"fusion/empty_variant.qxs", invalid_fixture_query::variant_info, invalid_fixture_diagnostic::semantic,
         "VARIANT must declare at least one alternative"},
        {"fusion/duplicate_variant_type.qxs", invalid_fixture_query::variant_info, invalid_fixture_diagnostic::semantic,
         "VARIANT repeats alternative type"},
        {"fusion/valueless_default_with_default_variant.qxs", invalid_fixture_query::variant_info, invalid_fixture_diagnostic::semantic,
         "VALUELESS_DEFAULT cannot be combined with a DEFAULT alternative"},
        {"fusion/multiple_variant_defaults.qxs", invalid_fixture_query::variant_info, invalid_fixture_diagnostic::semantic,
         "VARIANT declares more than one DEFAULT alternative"},
        {"fusion/unknown_modifier.qxs", invalid_fixture_query::module_ast, invalid_fixture_diagnostic::syntax, "Unknown fusion keyword"},
        {"match/non_exhaustive_union.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "MATCH does not cover alternative"},
        {"match/missing_otherwise.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "MATCH alternative with WHERE requires OTHERWISE or a whole MATCH DEFAULT"},
        {"match/otherwise_before_where.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "MATCH OTHERWISE requires a preceding WHERE arm"},
        {"match/duplicate_otherwise.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "MATCH alternative may contain only one OTHERWISE arm"},
        {"match/union_with_type_selector.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "UNION MATCH arms must use CASE selectors"},
        {"match/variant_with_case_selector.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "VARIANT MATCH arms must use TYPE selectors"},
        {"match/unknown_union_selector.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic, "has no option named missing"},
        {"match/unknown_variant_selector.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "has no alternative of type"},
        {"match/shadow_non_bare_subject.qxs", invalid_fixture_query::module_ast, invalid_fixture_diagnostic::syntax,
         "MATCH SHADOW requires a bare identifier subject"},
        {"match/unwrap_into_void.qxs", invalid_fixture_query::vm_routine, invalid_fixture_diagnostic::semantic,
         "UNWRAP cannot produce a reference to a VOID alternative"},
    };

    std::filesystem::path const fixture_root = std::filesystem::path(QUXLANG_TESTS_TESTDDATA_PATH) / "querygraph_invalid";
    for (invalid_fixture const& fixture : fixtures)
    {
        SCOPED_TRACE(fixture.relative_path.string());
        std::filesystem::path const fixture_path = fixture_root / fixture.relative_path;
        std::ifstream source_stream(fixture_path, std::ios::binary);
        ASSERT_TRUE(source_stream.is_open()) << "Unable to open invalid source fixture: " << fixture_path;
        std::string const source(std::istreambuf_iterator< char >(source_stream), std::istreambuf_iterator< char >{});

        quxlang::source_bundle bundle = make_single_main_source_bundle(std::string{});
        bundle.module_sources.at("main_x64").files.at("main.qxs") = quxlang::source_file{.contents = source};
        quxlang::compiler_querygraph graph = make_x64_graph(bundle);
        quxlang::type_symbol const bad = quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "bad"};

        bool rejected = false;
        try
        {
            switch (fixture.query)
            {
            case invalid_fixture_query::module_ast:
                (void)graph.make_request< quxlang::module_ast_query >("main");
                break;
            case invalid_fixture_query::union_info:
                (void)graph.make_request< quxlang::union_info_query >(bad);
                break;
            case invalid_fixture_query::variant_info:
                (void)graph.make_request< quxlang::variant_info_query >(bad);
                break;
            case invalid_fixture_query::vm_routine:
            {
                std::optional< quxlang::instanciation_reference > const inst = graph.make_request< quxlang::instanciation_query >(
                    quxlang::initialization_reference{.initializee = bad});
                ASSERT_TRUE(inst.has_value()) << "Invalid routine fixture did not resolve ::bad";
                (void)graph.make_request< quxlang::vm_procedure3_query >(*inst);
                break;
            }
            }
        }
        catch (quxlang::compilation_error const& error)
        {
            rejected = true;
            bool const diagnostic_matches = fixture.diagnostic == invalid_fixture_diagnostic::syntax
                                                ? quxlang::typeis< quxlang::syntax_error >(error.structured_error)
                                                : quxlang::typeis< quxlang::semantic_error >(error.structured_error);
            EXPECT_TRUE(diagnostic_matches) << "Unexpected diagnostic kind: " << error.what();
            EXPECT_NE(std::string(error.what()).find(fixture.expected_message), std::string::npos) << error.what();
        }
        catch (std::exception const& error)
        {
            ADD_FAILURE() << "Invalid fixture raised a non-compilation exception: " << error.what();
        }
        EXPECT_TRUE(rejected) << "Invalid fixture was accepted";
    }
}

TEST(querygraph_queries, enum_info_rejects_conflicting_semantics)
{
    auto expect_bad_enum = [](std::string const& source)
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle(source);
        quxlang::compiler_querygraph graph = make_x64_graph(bundle);
        quxlang::type_symbol bad = quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "bad"};
        EXPECT_THROW(graph.make_request< quxlang::enum_info_query >(bad), std::logic_error);
    };

    expect_bad_enum("::bad ENUM [none = NULL, x DEFAULT];");
    expect_bad_enum("::bad ENUM [a DEFAULT, b DEFAULT];");
    expect_bad_enum("::bad ENUM [RESERVED FROM(1) TO(2), a = 1];");
}

TEST(querygraph_queries, flagset_info_allocates_implicit_bits_around_reserved_masks)
{
    quxlang::source_bundle bundle = make_single_main_source_bundle(R"(
::permissions FLAGSET [read, write, RESERVED = 12, exec] {
    ::read_write STATIC permissions := read #|| write;
}
)");
    quxlang::compiler_querygraph graph = make_x64_graph(bundle);
    quxlang::type_symbol main = quxlang::absolute_module_reference{"main"};
    quxlang::type_symbol permissions = quxlang::subsymbol{main, "permissions"};
    quxlang::type_symbol read = quxlang::subsymbol{permissions, "read"};
    quxlang::type_symbol read_write = quxlang::subsymbol{permissions, "read_write"};

    quxlang::flagset_info info = graph.make_request< quxlang::flagset_info_query >(permissions);

    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(permissions), quxlang::symbol_kind::class_);
    ASSERT_EQ(graph.make_request< quxlang::class_type_query >(permissions), quxlang::class_kind::flagset);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(read), quxlang::symbol_kind::flagset_value);
    ASSERT_EQ(graph.make_request< quxlang::symbol_type_query >(read_write), quxlang::symbol_kind::global_variable);
    EXPECT_EQ(info.bits, 5);
    EXPECT_EQ(info.storage_bytes, 1);
    ASSERT_EQ(info.values.size(), 3);
    ASSERT_EQ(info.reserved_masks.size(), 1);
    EXPECT_EQ(info.reserved_masks.at(0).mask, 12);
    EXPECT_EQ(info.reserved_bit_mask, 12);
    EXPECT_EQ(info.canonical_bit_mask, 19);

    std::map< std::string, quxlang::flagset_value_info > values;
    for (quxlang::flagset_value_info const& value : info.values)
    {
        values[value.name] = value;
    }

    EXPECT_EQ(values.at("read").mask, 1);
    EXPECT_EQ(values.at("write").mask, 2);
    EXPECT_EQ(values.at("exec").mask, 16);

    quxlang::class_placement_info placement = graph.make_request< quxlang::class_placement_info_query >(permissions);
    EXPECT_EQ(placement.size, 1);
    EXPECT_EQ(placement.alignment, 1);
}

TEST(querygraph_queries, flagset_info_rejects_overlapping_canonical_and_reserved_bits)
{
    auto expect_bad_flagset = [](std::string const& source)
    {
        quxlang::source_bundle bundle = make_single_main_source_bundle(source);
        quxlang::compiler_querygraph graph = make_x64_graph(bundle);
        quxlang::type_symbol bad = quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "bad"};
        EXPECT_THROW(graph.make_request< quxlang::flagset_info_query >(bad), std::logic_error);
    };

    expect_bad_flagset("::bad FLAGSET [a = 3, b = 1];");
    expect_bad_flagset("::bad FLAGSET [RESERVED = 2, a = 3];");
    expect_bad_flagset("::bad FLAGSET [a = 0];");
}

TEST(querygraph_queries, interface_symbols_slots_and_implementation_map)
{
    auto bundle = make_single_main_source_bundle(R"(
::iface INTERFACE DEFAULTABLE {
    .value FUNCTION(%x I32): I32;
    .fallback FUNCTION(%x I32): I32 { RETURN x + 10; }
    .pick FUNCTION(%x I32): I32;
    .pick FUNCTION(%x I64): I64;
}
::impl IMPLEMENTATION(iface) {
    ::value FUNCTION(%x I32): I32 { RETURN x + 1; }
    ::fallback FUNCTION(%x I32): I32 { RETURN x + 2; }
    ::pick FUNCTION(%x I32): I32 { RETURN x + 3; }
    ::pick FUNCTION(%x I64): I64 { RETURN x + 4; }
}
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto iface = quxlang::type_symbol(quxlang::subsymbol{main, "iface"});
    auto impl = quxlang::type_symbol(quxlang::subsymbol{main, "impl"});

    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(iface), quxlang::symbol_kind::interface_);
    EXPECT_EQ(graph.make_request< quxlang::symbol_type_query >(impl), quxlang::symbol_kind::implementation_);
    EXPECT_TRUE(graph.make_request< quxlang::interface_defaultable_query >(iface));
    EXPECT_EQ(graph.make_request< quxlang::implementation_interface_type_query >(impl), iface);

    auto placement = graph.make_request< quxlang::class_placement_info_query >(iface);
    auto machine = graph.make_request< quxlang::machine_info_query >(std::monostate{});
    EXPECT_EQ(placement.size, machine.pointer_size_bytes());
    EXPECT_EQ(placement.alignment, machine.pointer_align());

    auto slots = graph.make_request< quxlang::interface_slot_list_query >(iface);
    ASSERT_EQ(slots.size(), 4);
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto i64 = quxlang::type_symbol(quxlang::int_type{64, true});
    quxlang::interface_slot_key value_key{.name = "value", .concrete_params = quxlang::invotype{.positional = {i32}}, .concrete_return_type = i32};
    quxlang::interface_slot_key pick_i64_key{.name = "pick", .concrete_params = quxlang::invotype{.positional = {i64}}, .concrete_return_type = i64};

    auto has_value_slot = std::ranges::any_of(slots,
                                              [&](quxlang::interface_slot const& slot)
                                              {
                                                  return slot.key == value_key && !slot.declaration.has_default_body;
                                              });
    auto has_pick_i64_slot = std::ranges::any_of(slots,
                                                 [&](quxlang::interface_slot const& slot)
                                                 {
                                                     return slot.key == pick_i64_key;
                                                 });
    EXPECT_TRUE(has_value_slot);
    EXPECT_TRUE(has_pick_i64_slot);

    auto functions = graph.make_request< quxlang::implementation_function_map_query >(impl);
    ASSERT_EQ(functions.size(), slots.size());
    ASSERT_TRUE(functions.contains(value_key));
    EXPECT_NE(quxlang::to_string(functions.at(value_key)).find("impl::value"), std::string::npos);

    auto get_impl = quxlang::type_symbol(quxlang::subsymbol{impl, "GET_INTERFACE_IMPL"});
    auto get_impl_inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
        .initializee = get_impl,
        .parameters = {},
        .adaptations = quxlang::allowed_adaptations::destination_rebinding,
    });
    ASSERT_TRUE(get_impl_inst.has_value());
    auto routine = graph.make_request< quxlang::vm_procedure3_query >(*get_impl_inst);
    quxlang::vmir2::assembler asm_printer(routine);
    EXPECT_NE(asm_printer.to_string(routine).find("INTERFACE_INIT"), std::string::npos);
}

TEST(querygraph_queries, interface_slot_validation_rejects_bad_shapes)
{
    auto expect_bad_interface = [](std::string const& source)
    {
        auto bundle = make_single_main_source_bundle(source);
        auto graph = make_x64_graph(bundle);
        auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
        auto iface = quxlang::type_symbol(quxlang::subsymbol{main, "iface"});
        EXPECT_THROW(graph.make_request< quxlang::interface_slot_list_query >(iface), std::logic_error);
    };

    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x AUTO): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%...xs I32): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(@THIS I32): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x MissingType): I32; }");
    expect_bad_interface("::iface INTERFACE { .value FUNCTION(%x I32): I32; .value FUNCTION(%y I32): I32; }");
}

TEST(querygraph_queries, interface_implementation_map_rejects_missing_slot)
{
    auto bundle = make_single_main_source_bundle(R"(
::iface INTERFACE { .value FUNCTION(): I32; }
::bad_impl IMPLEMENTATION(iface) { }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto bad_impl = quxlang::type_symbol(quxlang::subsymbol{main, "bad_impl"});

    EXPECT_THROW(graph.make_request< quxlang::implementation_function_map_query >(bad_impl), std::logic_error);
}

TEST(querygraph_queries, option_number_and_bool_values_are_constexpr_literals)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::enabled OPTION BOOL DEFAULT_VALUE(FALSE);");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "7";
    bundle.targets.at("x64").module_configurations.at("main").option_values["enabled"] = "true";
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"answer"}},
        .context = context,
    });
    auto enabled = graph.make_request< quxlang::constexpr_bool_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"enabled"}},
        .context = context,
    });

    ASSERT_EQ(answer, 7);
    ASSERT_TRUE(enabled);
}

TEST(querygraph_queries, constexpr_eval_v3_discards_primary_result_when_no_type_expected)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(quxlang::constexpr_input_v3{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = std::nullopt,
    });

    ASSERT_FALSE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_FALSE(result.deduced_type.has_value());
}

TEST(querygraph_queries, constexpr_eval_v3_auto_reports_deduced_primary_type)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(quxlang::constexpr_input_v3{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = quxlang::auto_temploidic{},
    });

    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_EQ(result.deduced_type, std::optional< quxlang::type_symbol >(quxlang::bool_type{}));
}

TEST(querygraph_queries, constexpr_eval_v3_scoped_typedef_resolves_type_expression)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_same_types{.lhs_type = quxlang::freebound_identifier{"T"}, .rhs_type = i32},
        .context = context,
        .expected_result_type = quxlang::bool_type{},
    };
    input.scoped_definitions["T"] = quxlang::scoped_typedef{.type = i32};

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    auto const& value = quxlang::constexpr_value_as_antestatal(result.values.at(quxlang::constexpr_primary_result_id));
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(value));
    ASSERT_EQ(quxlang::as< quxlang::antestatal_primitive >(value).value, (std::vector{std::byte{1}}));
}

TEST(querygraph_queries, constexpr_eval_v3_scoped_static_resolves_localdata)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto static_symbol = quxlang::static_local_ref{.functanoid = context, .name = "x", .generation = 1};

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"x"}},
        .context = context,
        .expected_result_type = i32,
    };
    input.scoped_definitions["x"] = quxlang::scoped_static{.symbol = static_symbol};
    input.statics[static_symbol] = quxlang::constexpr_static{
        .type = i32,
        .value = test_i32_value(std::byte{7}),
        .mutation_result_id = std::nullopt,
    };

    quxlang::constexpr_routine_v3_result generated = graph.make_request< quxlang::constexpr_routine_v3_query >(input);
    ASSERT_TRUE(generated.static_dependencies.contains(static_symbol));

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_TRUE(result.values.contains(quxlang::constexpr_primary_result_id));
    assert_i32_value(result.values.at(quxlang::constexpr_primary_result_id), std::byte{7});
}

TEST(querygraph_queries, constexpr_eval_v3_static_mutation_result_id_returns_static_value)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto static_symbol = quxlang::static_local_ref{.functanoid = context, .name = "x", .generation = 1};

    quxlang::constexpr_input_v3 input{
        .expr = quxlang::expression_value_keyword{"TRUE"},
        .context = context,
        .expected_result_type = std::nullopt,
    };
    input.statics[static_symbol] = quxlang::constexpr_static{
        .type = i32,
        .value = test_i32_value(std::byte{9}),
        .mutation_result_id = 17,
    };

    auto result = graph.make_request< quxlang::constexpr_eval_v3_query >(input);
    ASSERT_FALSE(result.values.contains(quxlang::constexpr_primary_result_id));
    ASSERT_TRUE(result.values.contains(17));
    assert_i32_value(result.values.at(17), std::byte{9});
}

TEST(querygraph_queries, option_number_uses_default_when_unconfigured)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4);");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"answer"}},
        .context = context,
    });

    ASSERT_EQ(answer, 4);
}

TEST(querygraph_queries, option_string_value_is_codegen_literal)
{
    auto bundle = make_single_main_source_bundle("::message OPTION STRING DEFAULT_VALUE(\"fallback\");");
    bundle.targets.at("x64").module_configurations.at("main").option_values["message"] = "configured";
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    quxlang::constexpr_routine_result routine_result = graph.make_request< quxlang::constexpr_routine_query >(quxlang::constexpr_input2{
        .expr = quxlang::expression_symbol_reference{quxlang::freebound_identifier{"message"}},
        .context = context,
        .type = quxlang::readonly_constant{.kind = quxlang::constant_kind::string},
    });
    quxlang::vmir2::functanoid_routine3 const& routine = routine_result.routine;

    std::vector< std::byte > expected;
    for (char const c : std::string("configured"))
    {
        expected.push_back(static_cast< std::byte >(c));
    }

    bool found_string_load = false;
    for (auto const& block : routine.blocks)
    {
        for (auto const& instruction : block.instructions)
        {
            if (quxlang::typeis< quxlang::vmir2::load_const_value >(instruction))
            {
                found_string_load = found_string_load || quxlang::as< quxlang::vmir2::load_const_value >(instruction).value == expected;
            }
        }
    }

    ASSERT_TRUE(found_string_load);
}

TEST(querygraph_queries, constexpr_routine_generated_ir_inherits_expression_location)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);
    auto context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    quxlang::source_location loc{.file_id = 7, .begin_index = 3, .end_index = std::optional< std::size_t >{5}};
    quxlang::expression expr = quxlang::expression_numeric_literal{.value = "42", .location = loc};

    quxlang::constexpr_routine_result routine_result = graph.make_request< quxlang::constexpr_routine_query >(quxlang::constexpr_input2{
        .expr = expr,
        .context = context,
        .type = quxlang::int_type{.bits = 64, .has_sign = false},
    });
    quxlang::vmir2::functanoid_routine3 const& routine = routine_result.routine;

    auto expect_location = [&](std::optional< quxlang::source_location > location)
    {
        ASSERT_TRUE(location.has_value());
        ASSERT_EQ(location->file_id, loc.file_id);
        ASSERT_EQ(location->begin_index, loc.begin_index);
        ASSERT_EQ(location->end_index, loc.end_index);
    };

    bool found_instruction = false;
    bool found_terminator = false;
    for (auto const& block : routine.blocks)
    {
        for (auto const& instruction : block.instructions)
        {
            expect_location(quxlang::vmir2::get_location(instruction));
            found_instruction = true;
        }

        if (block.terminator.has_value())
        {
            expect_location(quxlang::vmir2::get_location(*block.terminator));
            found_terminator = true;
        }
    }

    ASSERT_TRUE(found_instruction);
    ASSERT_TRUE(found_terminator);
}

TEST(querygraph_queries, option_default_from_copies_another_option)
{
    auto bundle = make_single_main_source_bundle("::answer OPTION NUMBER DEFAULT_VALUE(4); ::holder STRUCT { .answer OPTION NUMBER DEFAULT_FROM(::answer); }");
    bundle.targets.at("x64").module_configurations.at("main").option_values["answer"] = "11";
    auto graph = make_x64_graph(bundle);
    auto holder = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "holder"});
    auto holder_answer = quxlang::type_symbol(quxlang::submember{holder, "answer"});

    auto answer = graph.make_request< quxlang::constexpr_u64_query >(quxlang::constexpr_input{
        .expr = quxlang::expression_symbol_reference{holder_answer},
        .context = quxlang::type_symbol(quxlang::absolute_module_reference{"main"}),
    });

    ASSERT_EQ(answer, 11);
}

TEST(querygraph_queries, type_is_antestatal_accepts_conservative_shapes)
{
    auto bundle = make_single_main_source_bundle("::main VAR I32;");
    auto graph = make_x64_graph(bundle);

    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    quxlang::procedure_type proc;
    auto proc_type = quxlang::type_symbol(proc);
    auto proc_ptr = quxlang::type_symbol(quxlang::ptrref_type{.target = proc_type, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut});
    auto i32_array = quxlang::type_symbol(quxlang::array_type{.element_type = i32, .element_count = quxlang::expression_numeric_literal{"4"}});
    auto storage_type = quxlang::type_symbol(quxlang::storage{.storable_types = {i32}});

    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(i32));
    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(i32_array));
    ASSERT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(storage_type));
    ASSERT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(proc_type));
    ASSERT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(proc_ptr));
}

TEST(querygraph_queries, type_is_trivially_default_constructible_accepts_zero_constructible_shapes)
{
    auto bundle = make_single_main_source_bundle(R"(
::zero_enum ENUM [zero = 0 DEFAULT, one = 1];
::nonzero_enum ENUM [one = 1 DEFAULT, zero = 0];
::flags FLAGSET [read, write];
::trivial_record STRUCT { .x VAR I32; .values VAR [3]I32; }
::blocked_record STRUCT NO_IMPLICIT_DEFAULT_CONSTRUCTOR { .x VAR I32; }
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto f32 = quxlang::type_symbol(quxlang::float_type{32});
    quxlang::procedure_type procedure;
    procedure.signature.return_type = i32;
    auto procedure_type = quxlang::type_symbol(procedure);
    auto pointer_type = quxlang::type_symbol(quxlang::ptrref_type{.target = i32, .ptr_class = quxlang::pointer_class::instance, .qual = quxlang::qualifier::mut});
    auto storage_type = quxlang::type_symbol(quxlang::storage{.storable_types = {i32}});
    auto i32_array = quxlang::type_symbol(quxlang::array_type{.element_type = i32, .element_count = quxlang::expression_numeric_literal{"4"}});
    auto custom_array = quxlang::type_symbol(quxlang::array_type{.element_type = quxlang::subsymbol{main, "custom_record"}, .element_count = quxlang::expression_numeric_literal{"2"}});

    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(i32));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::bool_type{}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(f32));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(pointer_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(procedure_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(storage_type));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(i32_array));

    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "flags"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "zero_enum"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "nonzero_enum"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "trivial_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "blocked_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(quxlang::subsymbol{main, "custom_record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_default_constructible_query >(custom_array));
}

TEST(querygraph_queries, type_is_trivially_relocatable_accepts_nominal_integer_shapes)
{
    auto bundle = make_single_main_source_bundle(R"(
::zero_enum ENUM [zero = 0 DEFAULT, one = 1];
::nonzero_enum ENUM [one = 1 DEFAULT, zero = 0];
::flags FLAGSET [read, write];
::record STRUCT { .x VAR I32; }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});
    auto i32_array = quxlang::type_symbol(quxlang::array_type{.element_type = i32, .element_count = quxlang::expression_numeric_literal{"4"}});
    auto record_array = quxlang::type_symbol(quxlang::array_type{.element_type = quxlang::subsymbol{main, "record"}, .element_count = quxlang::expression_numeric_literal{"2"}});

    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(i32));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(i32_array));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(quxlang::subsymbol{main, "flags"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(quxlang::subsymbol{main, "zero_enum"}));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(quxlang::subsymbol{main, "nonzero_enum"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(quxlang::subsymbol{main, "record"}));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_trivially_relocatable_query >(record_array));
    EXPECT_THROW((void)graph.make_request< quxlang::type_is_trivially_relocatable_query >(quxlang::size_type{}), quxlang::compiler_bug);
}

TEST(querygraph_queries, global_init_type_classifies_default_trivial_globals)
{
    auto bundle = make_single_main_source_bundle(R"(
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::trivial_global VAR I32;
::trivial_array_global VAR [2]I32;
::explicit_global VAR I32 := 0;
::custom_global VAR custom_record;
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "trivial_global"}), quxlang::initialization_type::init_trivial);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "trivial_array_global"}), quxlang::initialization_type::init_trivial);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "explicit_global"}), quxlang::initialization_type::init_with_guard);
    EXPECT_EQ(graph.make_request< quxlang::global_init_type_query >(quxlang::subsymbol{main, "custom_global"}), quxlang::initialization_type::init_with_guard);
}

TEST(querygraph_queries, global_get_reference_omits_initguard_for_trivial_globals)
{
    auto bundle = make_single_main_source_bundle(R"(
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::trivial_global VAR I32;
::custom_global VAR custom_record;
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    auto routine_text = [&](std::string const& name)
    {
        quxlang::type_symbol const global = quxlang::subsymbol{main, name};
        auto inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
            .initializee = quxlang::submember{global, "GET_REFERENCE"},
            .parameters = {},
            .adaptations = quxlang::allowed_adaptations::destination_rebinding,
        });
        EXPECT_TRUE(inst.has_value());
        quxlang::vmir2::functanoid_routine3 routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
        return quxlang::vmir2::assembler(routine).to_string(routine);
    };

    std::string const trivial_text = routine_text("trivial_global");
    EXPECT_NE(trivial_text.find("GET_OBJECT_REF GLOBAL, OBJECT"), std::string::npos);
    EXPECT_EQ(trivial_text.find("GET_OBJECT_REF GLOBAL, STORAGE"), std::string::npos);
    EXPECT_EQ(trivial_text.find("STORAGE_PUN"), std::string::npos);
    EXPECT_EQ(trivial_text.find("INITGUARD_TRY_ACQUIRE"), std::string::npos);
    EXPECT_EQ(trivial_text.find("CALL"), std::string::npos);

    std::string const custom_text = routine_text("custom_global");
    EXPECT_NE(custom_text.find("INITGUARD_TRY_ACQUIRE"), std::string::npos);
}

TEST(querygraph_queries, global_get_reference_uses_thread_access_for_per_thread_globals)
{
    auto bundle = make_single_main_source_bundle(R"(
::custom_record STRUCT { .x VAR I32; .CONSTRUCTOR FUNCTION() { } }
::plain_global VAR I32;
::thread_trivial PER_THREAD VAR I32;
::thread_custom PER_THREAD VAR custom_record;
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});

    EXPECT_FALSE(graph.make_request< quxlang::global_is_per_thread_query >(quxlang::subsymbol{main, "plain_global"}));
    EXPECT_TRUE(graph.make_request< quxlang::global_is_per_thread_query >(quxlang::subsymbol{main, "thread_trivial"}));
    EXPECT_TRUE(graph.make_request< quxlang::global_is_per_thread_query >(quxlang::subsymbol{main, "thread_custom"}));

    auto routine_text = [&](std::string const& name)
    {
        quxlang::type_symbol const global = quxlang::subsymbol{main, name};
        auto inst = graph.make_request< quxlang::instanciation_query >(quxlang::initialization_reference{
            .initializee = quxlang::submember{global, "GET_REFERENCE"},
            .parameters = {},
            .adaptations = quxlang::allowed_adaptations::destination_rebinding,
        });
        EXPECT_TRUE(inst.has_value());
        quxlang::vmir2::functanoid_routine3 routine = graph.make_request< quxlang::vm_procedure3_query >(*inst);
        return quxlang::vmir2::assembler(routine).to_string(routine);
    };

    std::string const trivial_text = routine_text("thread_trivial");
    EXPECT_NE(trivial_text.find("GET_OBJECT_REF THREAD, OBJECT"), std::string::npos);
    EXPECT_EQ(trivial_text.find("GET_OBJECT_REF GLOBAL"), std::string::npos);
    EXPECT_EQ(trivial_text.find("INITGUARD_TRY_ACQUIRE"), std::string::npos);

    std::string const custom_text = routine_text("thread_custom");
    EXPECT_NE(custom_text.find("INITGUARD_TRY_ACQUIRE THREAD"), std::string::npos);
    EXPECT_NE(custom_text.find("GET_OBJECT_REF THREAD, STORAGE"), std::string::npos);
    EXPECT_EQ(custom_text.find("GET_OBJECT_REF GLOBAL"), std::string::npos);
}

TEST(querygraph_queries, output_llvm_marks_per_thread_global_thread_local)
{
    auto bundle = make_single_main_source_bundle(R"(
::bif PER_THREAD VAR I32;

::main FUNCTION(): I32
{
  RETURN bif;
}
)");
    auto graph = make_x64_graph(bundle);

    std::string const llvm_ir = graph.make_request< quxlang::output_unoptimized_llvm_query >("default");

    EXPECT_NE(llvm_ir.find("thread_local(localexec) global i32 0"), std::string::npos);
}

TEST(querygraph_queries, static_struct_keywords_and_user_deserialize_constructor)
{
    auto bundle = make_single_main_source_bundle(R"(
::plain STRUCT { .x VAR I32; }
::explicit_antestatal STRUCT ANTESTATAL { .x VAR I32; }
::explicit_serialoid STRUCT SERIALOID { .x VAR I32; }
::default_serialoid STRUCT {
    .x VAR I32;
    .SERIALIZE FUNCTION(@OUTPUT_ITERATOR:output =>> BYTE): =>> BYTE { RETURN output; }
    .CONSTRUCTOR FUNCTION(@DESERIALIZE_INPUT_ITERATOR:input =>> BYTE) { }
}
::plain_static STATIC plain;
::serial_static STATIC default_serialoid := default_serialoid();
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto plain = quxlang::type_symbol(quxlang::subsymbol{main, "plain"});
    auto explicit_antestatal = quxlang::type_symbol(quxlang::subsymbol{main, "explicit_antestatal"});
    auto explicit_serialoid = quxlang::type_symbol(quxlang::subsymbol{main, "explicit_serialoid"});
    auto default_serialoid = quxlang::type_symbol(quxlang::subsymbol{main, "default_serialoid"});
    auto plain_static = quxlang::type_symbol(quxlang::subsymbol{main, "plain_static"});
    auto serial_static = quxlang::type_symbol(quxlang::subsymbol{main, "serial_static"});

    EXPECT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(plain));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_antestatal_query >(explicit_antestatal));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(explicit_serialoid));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_serialoid_query >(explicit_serialoid));

    EXPECT_TRUE(graph.make_request< quxlang::user_deserialize_exists_query >(default_serialoid));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_antestatal_query >(default_serialoid));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_serialoid_query >(default_serialoid));

    EXPECT_TRUE(graph.make_request< quxlang::global_is_antestatal_static_query >(plain_static));
    EXPECT_TRUE(graph.make_request< quxlang::global_is_serialoid_static_query >(serial_static));
}

TEST(querygraph_queries, type_is_stringlike_requires_struct_tag)
{
    auto bundle = make_single_main_source_bundle(R"(
::plain STRUCT { .x VAR I32; }
::textish STRUCT STRINGLIKE { .x VAR I32; }
)");
    auto graph = make_x64_graph(bundle);
    auto main = quxlang::type_symbol(quxlang::absolute_module_reference{"main"});
    auto plain = quxlang::type_symbol(quxlang::subsymbol{main, "plain"});
    auto textish = quxlang::type_symbol(quxlang::subsymbol{main, "textish"});
    auto i32 = quxlang::type_symbol(quxlang::int_type{32, true});

    EXPECT_FALSE(graph.make_request< quxlang::type_is_stringlike_query >(plain));
    EXPECT_TRUE(graph.make_request< quxlang::type_is_stringlike_query >(textish));
    EXPECT_FALSE(graph.make_request< quxlang::type_is_stringlike_query >(i32));
}

TEST(querygraph_queries, global_static_i32_is_antestatal_and_materializes_value)
{
    auto bundle = make_single_main_source_bundle("::foo STATIC I32 := 4;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    ASSERT_TRUE(graph.make_request< quxlang::global_is_antestatal_static_query >(foo));

    auto value = graph.make_request< quxlang::antestatal_static_value_query >(foo);
    ASSERT_TRUE(quxlang::typeis< quxlang::antestatal_primitive >(value));
    auto const& bytes = quxlang::as< quxlang::antestatal_primitive >(value).value;
    ASSERT_GE(bytes.size(), 1);
    ASSERT_EQ(std::to_integer< std::uint32_t >(bytes.at(0)), 4);

}

TEST(querygraph_queries, nonstatic_global_is_not_antestatal_static)
{
    auto bundle = make_single_main_source_bundle("::foo VAR I32 := 4;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    ASSERT_FALSE(graph.make_request< quxlang::global_is_antestatal_static_query >(foo));
}

TEST(querygraph_queries, nonstatic_static_global_is_rejected)
{
    auto bundle = make_single_main_source_bundle("::blocked STRUCT NONSTATIC { .x VAR I32; } ::foo STATIC blocked;");
    auto graph = make_x64_graph(bundle);
    auto foo = quxlang::type_symbol(quxlang::subsymbol{quxlang::absolute_module_reference{"main"}, "foo"});

    EXPECT_THROW(graph.make_request< quxlang::global_is_antestatal_static_query >(foo), std::logic_error);
    EXPECT_THROW(graph.make_request< quxlang::global_is_serialoid_static_query >(foo), std::logic_error);
}

TEST(querygraph_queries, vmir_fusion_instruction_assembly_round_trips)
{
    std::vector< std::string > const sources{
        "FUSION_ACTIVE_INDEX %1, %2",
        "FUSION_HAS_ALTERNATIVE %1, #3, %2",
        "FUSION_IS_VALUELESS %1, %2",
        "FUSION_STORAGE_REF %1, #3, %2",
        "FUSION_SET_ACTIVE %1, #3",
        "FUSION_SET_ACTIVE %1, #3, %4",
        "FUSION_SET_VALUELESS %1",
        "FUSION_SWAP_BOXED_STATE %1, %2",
    };

    quxlang::vmir2::functanoid_routine3 routine;
    quxlang::vmir2::assembler printer(routine);
    for (std::string const& source : sources)
    {
        quxlang::parsers::parsing_context context = quxlang::parsers::make_unlocated_parsing_context(source);
        std::optional< quxlang::vmir2::vm_instruction > const instruction = quxlang::parsers::vmir2::try_parse_instruction(context);
        ASSERT_TRUE(instruction.has_value()) << source;
        EXPECT_EQ(context.iter_pos, context.iter_end) << source;
        EXPECT_EQ(printer.to_string(*instruction), source);
    }
}

TEST(querygraph_queries, vmir_tablebranch_assembly_and_reachability_include_every_target)
{
    std::string const source = "TABLEBRANCH %0, [!1, !2, !3], !4";
    quxlang::parsers::parsing_context context = quxlang::parsers::make_unlocated_parsing_context(source);
    std::optional< quxlang::vmir2::vm_terminator > const parsed = quxlang::parsers::vmir2::try_parse_terminator(context);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->type_is< quxlang::vmir2::tablebranch >());
    EXPECT_EQ(context.iter_pos, context.iter_end);

    quxlang::vmir2::functanoid_routine3 routine;
    routine.blocks.resize(5);
    routine.blocks[0].terminator = *parsed;
    for (std::size_t i = 1; i < routine.blocks.size(); ++i)
    {
        routine.blocks[i].terminator = quxlang::vmir2::ret{};
    }
    EXPECT_EQ(quxlang::vmir2::assembler(routine).to_string(*parsed), source);

    std::set< quxlang::vmir2::block_index > const reachable = quxlang::vmir2::reachable_blocks(routine, quxlang::dependency_set::native);
    EXPECT_EQ(reachable, (std::set< quxlang::vmir2::block_index >{
                             quxlang::vmir2::block_index(0),
                             quxlang::vmir2::block_index(1),
                             quxlang::vmir2::block_index(2),
                             quxlang::vmir2::block_index(3),
                             quxlang::vmir2::block_index(4),
                         }));
}

TEST(querygraph_queries, vmir_panic_disconnects_local_and_snapshot_requirements)
{
    quxlang::type_symbol const owner = quxlang::absolute_module_reference{"reachability_test"};
    quxlang::static_snapshot_ref const reachable_snapshot{
        .functanoid = owner,
        .name = "reachable",
        .snapshot_id = 1,
    };
    quxlang::static_snapshot_ref const nested_snapshot{
        .functanoid = owner,
        .name = "nested",
        .snapshot_id = 2,
    };
    quxlang::static_snapshot_ref const unreachable_snapshot{
        .functanoid = owner,
        .name = "unreachable",
        .snapshot_id = 3,
    };
    quxlang::type_symbol const unreachable_type = quxlang::subsymbol{.of = owner, .name = "unreachable_type"};
    quxlang::type_symbol const abi_type = quxlang::subsymbol{.of = owner, .name = "abi_type"};
    quxlang::type_symbol const reachable_snapshot_type = quxlang::ptrref_type{
        .target = quxlang::bool_type{},
        .ptr_class = quxlang::pointer_class::instance,
        .qual = quxlang::qualifier::constant,
    };

    quxlang::instanciation_reference const unreachable_functanoid{
        .temploid = quxlang::temploid_reference{
            .templexoid = quxlang::subsymbol{.of = owner, .name = "unreachable_function"},
            .overload_id = 0,
        },
    };
    quxlang::type_symbol const unreachable_global = quxlang::subsymbol{.of = owner, .name = "unreachable_global"};
    quxlang::antestatal_interface unreachable_interface{
        .interface_type = quxlang::subsymbol{.of = owner, .name = "interface_type"},
        .functions = {{quxlang::interface_slot_key{.name = "invoke"}, quxlang::type_symbol(unreachable_functanoid)}},
    };
    quxlang::antestatal_struct unreachable_value{
        .fields = {
            {"function", quxlang::antestatal_value(std::move(unreachable_interface))},
            {"global", quxlang::antestatal_value(quxlang::antestatal_ptrref{
                           .target = quxlang::antestatal_access_global{.symbol = unreachable_global},
                       })},
        },
    };

    quxlang::vmir2::functanoid_routine3 routine;
    routine.local_types = {
        quxlang::vmir2::local_type{.type = quxlang::bool_type{}},
        quxlang::vmir2::local_type{.type = quxlang::make_cref(reachable_snapshot_type)},
        quxlang::vmir2::local_type{.type = quxlang::make_cref(unreachable_type)},
        quxlang::vmir2::local_type{.type = unreachable_type},
        quxlang::vmir2::local_type{.type = abi_type},
    };
    routine.parameters.named["RETURN"] = quxlang::vmir2::routine_parameter{
        .type = quxlang::nvalue_slot{.target = abi_type},
        .local_index = quxlang::vmir2::local_index(4),
    };
    routine.blocks.resize(2);
    routine.blocks[0].instructions = {
        quxlang::vmir2::load_const_bool{.target = quxlang::vmir2::local_index(0), .value = true},
        quxlang::vmir2::get_antestatal_ref{
            .symbol = quxlang::type_symbol(reachable_snapshot),
            .target_ref = quxlang::vmir2::local_index(1),
        },
    };
    routine.blocks[0].terminator = quxlang::vmir2::panic{.message = "stop"};
    routine.blocks[1].instructions = {
        quxlang::vmir2::get_antestatal_ref{
            .symbol = quxlang::type_symbol(unreachable_snapshot),
            .target_ref = quxlang::vmir2::local_index(2),
        },
        quxlang::vmir2::load_const_zero{.target = quxlang::vmir2::local_index(3)},
    };
    routine.blocks[1].terminator = quxlang::vmir2::ret{};
    routine.static_snapshots[reachable_snapshot] = quxlang::vmir2::localdata_entry{
        .type = reachable_snapshot_type,
        .value = quxlang::antestatal_ptrref{
            .target = quxlang::antestatal_access_global{.symbol = quxlang::type_symbol(nested_snapshot)},
        },
    };
    routine.static_snapshots[nested_snapshot] = quxlang::vmir2::localdata_entry{
        .type = quxlang::bool_type{},
        .value = quxlang::antestatal_primitive{},
    };
    routine.static_snapshots[unreachable_snapshot] = quxlang::vmir2::localdata_entry{
        .type = unreachable_type,
        .value = std::move(unreachable_value),
    };

    EXPECT_EQ(quxlang::vmir2::reachable_blocks(routine, quxlang::dependency_set::native),
              (std::set< quxlang::vmir2::block_index >{quxlang::vmir2::block_index(0)}));
    EXPECT_EQ(quxlang::vmir2::reachable_local_slots(routine, quxlang::dependency_set::native),
              (std::set< quxlang::vmir2::local_index >{
                  quxlang::vmir2::local_index(0),
                  quxlang::vmir2::local_index(1),
                  quxlang::vmir2::local_index(4),
              }));
    EXPECT_EQ(quxlang::vmir2::directly_required_static_snapshots(routine, quxlang::dependency_set::native),
              (std::set< quxlang::static_snapshot_ref >{reachable_snapshot, nested_snapshot}));

    std::set< quxlang::type_symbol > const placements = quxlang::vmir2::directly_required_type_placements(routine, quxlang::dependency_set::native);
    EXPECT_TRUE(placements.contains(abi_type));
    EXPECT_TRUE(placements.contains(reachable_snapshot_type));
    EXPECT_FALSE(placements.contains(unreachable_type));
    EXPECT_FALSE(quxlang::vmir2::directly_instantiated_functanoids(routine, quxlang::dependency_set::native)
                     .contains(quxlang::type_symbol(unreachable_functanoid)));
    EXPECT_FALSE(quxlang::vmir2::directly_referenced_antestatal_globals(routine, quxlang::dependency_set::native)
                     .contains(unreachable_global));
}

TEST(querygraph_queries, vmir_reachable_locals_follow_runtime_constexpr_mode)
{
    quxlang::vmir2::functanoid_routine3 routine;
    routine.local_types = {
        quxlang::vmir2::local_type{.type = quxlang::bool_type{}},
        quxlang::vmir2::local_type{.type = quxlang::int_type{.bits = 32, .has_sign = true}},
        quxlang::vmir2::local_type{.type = quxlang::byte_type{}},
    };
    routine.parameters.positional.push_back(quxlang::vmir2::routine_parameter{
        .type = quxlang::byte_type{},
        .local_index = quxlang::vmir2::local_index(2),
    });
    routine.blocks.resize(3);
    routine.blocks[0].terminator = quxlang::vmir2::runtime_constexpr{
        .target_constexpr = quxlang::vmir2::block_index(2),
        .target_native = quxlang::vmir2::block_index(1),
    };
    routine.blocks[1].instructions.push_back(quxlang::vmir2::load_const_bool{
        .target = quxlang::vmir2::local_index(0),
        .value = true,
    });
    routine.blocks[1].terminator = quxlang::vmir2::ret{};
    routine.blocks[2].instructions.push_back(quxlang::vmir2::load_const_zero{
        .target = quxlang::vmir2::local_index(1),
    });
    routine.blocks[2].terminator = quxlang::vmir2::ret{};

    EXPECT_EQ(quxlang::vmir2::reachable_local_slots(routine, quxlang::dependency_set::native),
              (std::set< quxlang::vmir2::local_index >{
                  quxlang::vmir2::local_index(0),
                  quxlang::vmir2::local_index(2),
              }));
    EXPECT_EQ(quxlang::vmir2::reachable_local_slots(routine, quxlang::dependency_set::constexpr_),
              (std::set< quxlang::vmir2::local_index >{
                  quxlang::vmir2::local_index(1),
                  quxlang::vmir2::local_index(2),
              }));
}

TEST(querygraph_queries, vmir_fusion_inspection_borrows_subject_and_records_layout)
{
    quxlang::type_symbol const fusion = quxlang::subsymbol{
        .of = quxlang::absolute_module_reference{"main"},
        .name = "fusion",
    };
    quxlang::type_symbol const fusion_ref = quxlang::ptrref_type{
        .target = fusion,
        .ptr_class = quxlang::pointer_class::ref,
        .qual = quxlang::qualifier::constant,
    };
    quxlang::vmir2::functanoid_routine3 routine;
    routine.local_types = {
        quxlang::vmir2::local_type{.type = fusion_ref},
        quxlang::vmir2::local_type{.type = quxlang::bool_type{}},
    };
    routine.blocks.resize(1);
    routine.blocks[0].entry_state[quxlang::vmir2::local_index(0)] = quxlang::vmir2::slot_state{
        .stage = quxlang::vmir2::slot_stage::full,
        .storage_valid = true,
    };
    routine.blocks[0].instructions.push_back(quxlang::vmir2::fusion_is_valueless{
        .subject = quxlang::vmir2::local_index(0),
        .result = quxlang::vmir2::local_index(1),
    });
    routine.blocks[0].terminator = quxlang::vmir2::ret{};

    quxlang::vmir2::state_map state = routine.blocks[0].entry_state;
    quxlang::vmir2::codegen_state_engine engine(state, routine.local_types, routine.parameters);
    engine.apply(routine.blocks[0].instructions.front());
    EXPECT_TRUE(state.at(quxlang::vmir2::local_index(0)).alive());
    EXPECT_TRUE(state.at(quxlang::vmir2::local_index(1)).alive());
    EXPECT_EQ(quxlang::vmir2::directly_required_fusion_layouts(routine, quxlang::dependency_set::native),
              (std::set< quxlang::type_symbol >{fusion}));
}
