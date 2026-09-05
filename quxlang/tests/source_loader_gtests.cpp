// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "source_loader_internal.hpp"

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <string>
#include <type_traits>


TEST(source_loader, reproducibility_error_is_not_a_compilation_error)
{
    static_assert(std::is_base_of_v< std::logic_error, quxlang::reproducibility_error >);
    static_assert(!std::is_base_of_v< quxlang::compilation_error, quxlang::reproducibility_error >);
}

TEST(source_loader, rejects_carriage_returns)
{
    EXPECT_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", "::main VAR I32;\r\n"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_byte_order_marks)
{
    std::string const source = "\xef\xbb\xbf::main VAR I32;\n";
    EXPECT_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", source), quxlang::reproducibility_error);
}

TEST(source_loader, accepts_portable_source_contents)
{
    EXPECT_NO_THROW(quxlang::detail::validate_source_file_contents("modules/main/sources/main.qxs", "::main VAR I32;\n"));
}

TEST(source_loader, rejects_paths_differing_only_in_capitalization)
{
    quxlang::detail::source_path_validator validator;
    validator.add("modules/main/sources/Main.qxs");
    EXPECT_THROW(validator.add("modules/main/sources/main.qxs"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_characters_illegal_on_a_major_filesystem)
{
    quxlang::detail::source_path_validator validator;
    EXPECT_THROW(validator.add("modules/main/sources/bad?.qxs"), quxlang::reproducibility_error);
}

TEST(source_loader, rejects_filename_components_over_portable_limit)
{
    quxlang::detail::source_path_validator validator;
    std::string const long_filename(252, 'a');
    EXPECT_THROW(validator.add(std::filesystem::path("modules/main/sources") / (long_filename + ".qxs")), quxlang::reproducibility_error);
}

TEST(source_loader, parses_ordered_cpu_stepping_attribute_forms)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_FEATURE_SSE2
- attributes:
    X64_FEATURE_AVX2: true
    X64_PERF_FAST_GATHER: false
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(2));
    EXPECT_FALSE(steppings.at(0).tune.has_value());
    EXPECT_FALSE(steppings.at(1).tune.has_value());
    EXPECT_EQ(steppings.at(0).attributes, (std::map< std::string, bool >{{"X64_FEATURE_SSE2", true}}));
    EXPECT_EQ(
        steppings.at(1).attributes,
        (std::map< std::string, bool >{{"X64_FEATURE_AVX2", true}, {"X64_PERF_FAST_GATHER", false}}));
}

TEST(source_loader, parses_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_VENDOR_AMD
  tune: X64_TUNE_AMD_ZEN4
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(1));
    ASSERT_TRUE(steppings.front().tune.has_value());
    EXPECT_EQ(*steppings.front().tune, "X64_TUNE_AMD_ZEN4");
}

TEST(source_loader, parses_historical_x86_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X86_VENDOR_AMD
  tune: X86_TUNE_AMD_ATHLON_XP
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_32, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(1));
    EXPECT_EQ(steppings.front().attributes, (std::map< std::string, bool >{{"X86_VENDOR_AMD", true}}));
    ASSERT_TRUE(steppings.front().tune.has_value());
    EXPECT_EQ(*steppings.front().tune, "X86_TUNE_AMD_ATHLON_XP");
}

TEST(source_loader, rejects_unknown_cpu_stepping_tuning_model)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes: []
  tune: X64_TUNE_AMD_NOT_REGISTERED
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, rejects_cpu_stepping_tuning_model_for_another_cpu)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes: []
  tune: X64_TUNE_INTEL_HASWELL
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::arm_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, preserves_cpu_attribute_group_constraints)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - X64_FEATURES_V1
- attributes:
    X64_FEATURES_V2: false
)YAML");

    std::vector< quxlang::cpu_stepping_configuration > const steppings =
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target");

    ASSERT_EQ(steppings.size(), static_cast< std::size_t >(2));
    EXPECT_EQ(steppings.at(0).attributes, (std::map< std::string, bool >{{"X64_FEATURES_V1", true}}));
    EXPECT_EQ(steppings.at(1).attributes, (std::map< std::string, bool >{{"X64_FEATURES_V2", false}}));
}

TEST(source_loader, rejects_cpu_stepping_attribute_for_another_cpu)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    - ARM64_FEATURE_ADVANCED_SIMD
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, rejects_disabled_attribute_at_stepping_zero)
{
    YAML::Node const node = YAML::Load(R"YAML(
- attributes:
    X64_FEATURE_SSE2: false
)YAML");

    EXPECT_THROW(
        quxlang::detail::parse_cpu_stepping_configurations(node, quxlang::cpu::x86_64, "Test target"),
        quxlang::compilation_error);
}

TEST(source_loader, parses_global_output_paths_and_target_backend_defaults)
{
    YAML::Node config = YAML::Load(R"YAML(
targets:
  native:
    platform: linux
    cpu: x64
    backend_llvm_options: {build_type: Debug}
    modules: {main: {source: main}}
  java:
    platform: jvm
    backend_cortado_options: {mode: standard}
    modules: {main: {source: main}}
outputs:
  bin/app:
    target: native
    type: executable
  bin/app-release:
    target: native
    type: executable
    backend_llvm_options: {build_type: Release}
  java/app.jar:
    target: java
    type: executable
    backend_cortado_options: {mode: address_sanitizer}
)YAML");
    quxlang::source_bundle bundle;
    quxlang::detail::parse_build_configuration(config, bundle, std::nullopt);
    ASSERT_EQ(bundle.targets.size(), 2U);
    ASSERT_EQ(bundle.outputs.size(), 3U);
    EXPECT_EQ(bundle.outputs.at("bin/app").target, "native");
    EXPECT_FALSE(bundle.outputs.at("bin/app").llvm_options.has_value());
    EXPECT_EQ(bundle.targets.at("native").llvm_options.build_type, quxlang::build_type::debug);
    ASSERT_TRUE(bundle.outputs.at("bin/app-release").llvm_options.has_value());
    EXPECT_EQ(bundle.outputs.at("bin/app-release").llvm_options->build_type, quxlang::build_type::release);
    EXPECT_EQ(bundle.outputs.at("java/app.jar").cortado_options->mode, quxlang::backend_cortado_mode::address_sanitizer);
}

TEST(source_loader, rejects_legacy_and_malformed_root_sections)
{
    for (std::string const& text : {"linux-x64: {platform: linux, cpu: x64}", "targets: {}", "outputs: {}", "targets: []\noutputs: {}", "targets: {}\noutputs: []", "targets: {}\noutputs: {}\nextra: {}", "targets: {}\noutputs: {}\noutputs: {}", "targets: {native: {platform: linux, cpu: x64, outputs: {}}}\noutputs: {}"})
    {
        SCOPED_TRACE(text);
        quxlang::source_bundle bundle;
        EXPECT_THROW(quxlang::detail::parse_build_configuration(YAML::Load(text), bundle, std::nullopt), quxlang::compilation_error);
    }
}

TEST(source_loader, rejects_invalid_output_configuration)
{
    std::string prefix = "targets:\n  native: {platform: linux, cpu: x64, modules: {main: {}}}\noutputs:\n  bin/app: ";
    for (std::string const& output : {"{}", "[]", "{type: executable}", "{target: native}", "{target: unknown, type: executable}", "{target: native, type: invalid}", "{target: native, type: executable, path: app}", "{target: native, type: executable, main_module: missing}", "{target: native, type: executable, test_modules: [main]}", "{target: native, type: unit_test_suite, main_module: main}", "{target: native, type: unit_test_suite, main_functanoid: main}", "{target: native, type: unit_test_suite, test_modules: []}", "{target: native, type: unit_test_suite, test_modules: [main, main]}", "{target: native, type: unit_test_suite, test_modules: [missing]}", "{target: native, type: executable, backend_cortado_options: {mode: standard}}"})
    {
        SCOPED_TRACE(output);
        quxlang::source_bundle bundle;
        EXPECT_THROW(quxlang::detail::parse_build_configuration(YAML::Load(prefix + output), bundle, std::nullopt), quxlang::compilation_error);
    }
}

TEST(source_loader, validates_global_outputs_before_target_selection)
{
    YAML::Node config = YAML::Load(R"YAML(
targets:
  native: {platform: linux, cpu: x64, modules: {main: {}}}
  java: {platform: jvm, modules: {main: {}}}
outputs:
  native/app: {target: native, type: executable}
  java/app.jar: {target: java, type: executable}
)YAML");
    std::optional< std::set< std::string > > selected = std::set< std::string >{"native"};
    quxlang::source_bundle bundle;
    quxlang::detail::parse_build_configuration(config, bundle, selected);
    ASSERT_EQ(bundle.targets.size(), 1U);
    ASSERT_EQ(bundle.outputs.size(), 1U);
    EXPECT_TRUE(bundle.outputs.contains("native/app"));

    config["outputs"]["java/app.jar"]["target"] = "missing";
    quxlang::source_bundle invalid_reference;
    EXPECT_THROW(quxlang::detail::parse_build_configuration(config, invalid_reference, selected), quxlang::compilation_error);

    config["outputs"]["java/app.jar"]["target"] = "java";
    config["outputs"]["NATIVE/app"] = config["outputs"]["java/app.jar"];
    quxlang::source_bundle collision;
    EXPECT_THROW(quxlang::detail::parse_build_configuration(config, collision, selected), quxlang::compilation_error);
}

TEST(source_loader, permits_targets_without_artifacts_and_rejects_unknown_selection)
{
    YAML::Node config = YAML::Load("targets: {native: {platform: linux, cpu: x64}}\noutputs: {}");
    quxlang::source_bundle bundle;
    quxlang::detail::parse_build_configuration(config, bundle, std::nullopt);
    EXPECT_TRUE(bundle.outputs.empty());
    EXPECT_TRUE(bundle.targets.at("native").run_static_tests);

    quxlang::source_bundle invalid;
    EXPECT_THROW(quxlang::detail::parse_build_configuration(config, invalid, std::set< std::string >{"missing"}), quxlang::compilation_error);
}

TEST(source_loader, rejects_nonportable_or_unnormalized_output_paths)
{
    std::vector< std::string > paths = {"", ".", "..", "/app", "../app", "bin/../app", "bin/./app", "bin//app", "bin/", "C:/app", "bin\\app", "app?", "app.", "app ", "NUL.exe", "bin/COM1", std::string("app\0bad", 7), std::string(256, 'a')};
    for (std::string const& path : paths)
    {
        SCOPED_TRACE(path);
        quxlang::detail::output_path_validator validator;
        EXPECT_THROW(validator.add(path), quxlang::compilation_error);
    }
}

TEST(source_loader, rejects_output_collisions_in_both_declaration_orders)
{
    for (std::pair< std::string, std::string > const& paths : {std::pair< std::string, std::string >{"bin/app", "bin/app"}, {"bin/app", "BIN/App"}, {"bin", "BIN/app"}, {"bin/app", "BIN"}})
    {
        quxlang::detail::output_path_validator validator;
        validator.add(paths.first);
        EXPECT_THROW(validator.add(paths.second), quxlang::compilation_error);
    }
    quxlang::detail::output_path_validator validator;
    EXPECT_NO_THROW(validator.add("bin/app"));
    EXPECT_NO_THROW(validator.add("bin/tests"));
    EXPECT_NO_THROW(validator.add("other/app"));
}

TEST(source_loader, rejects_duplicate_output_keys_in_yaml)
{
    YAML::Node config = YAML::Load(R"YAML(
targets:
  native: {platform: linux, cpu: x64, modules: {main: {}}}
outputs:
  app: {target: native, type: executable}
  app: {target: native, type: executable}
)YAML");
    quxlang::source_bundle bundle;
    EXPECT_THROW(quxlang::detail::parse_build_configuration(config, bundle, std::nullopt), quxlang::compilation_error);
}

TEST(source_loader, parses_all_build_types_and_rejects_legacy_llvm_mode)
{
    for (std::string name : {"Debug", "Quick", "Release", "DebugOpt", "DebugRelease", "Compact", "DebugCompact", "CompactOpt", "DebugCompactOpt"})
    {
        quxlang::build_type expected = quxlang::parse_build_type(name);
        std::string snake_case = rpnx::enum_traits< quxlang::build_type >::to_string(expected);
        std::string uppercase = snake_case;
        for (char& character : uppercase)
        {
            if (character >= 'a' && character <= 'z')
            {
                character -= 'a' - 'A';
            }
        }
        for (std::string spelling : {name, snake_case, uppercase})
        {
            YAML::Node config = YAML::Load("targets: {native: {platform: linux, cpu: x64, build_type: " + spelling + ", modules: {main: {source: main}}}}\noutputs: {app: {target: native, type: executable, build_type: " + spelling + ", backend_llvm_options: {build_type: " + spelling + "}}}");
            quxlang::source_bundle bundle;
            quxlang::detail::parse_build_configuration(config, bundle, std::nullopt);
            EXPECT_EQ(bundle.targets.at("native").build_type, expected);
            EXPECT_EQ(bundle.outputs.at("app").build_type, expected);
            EXPECT_EQ(bundle.outputs.at("app").llvm_options->build_type, expected);
        }
    }
    for (std::string field : {"build_type: unknown", "build_type: Fast", "backend_llvm_options: {mode: debug}", "backend_llvm_options: {build_type: optimized}"})
    {
        YAML::Node config = YAML::Load("targets: {native: {platform: linux, cpu: x64, modules: {main: {source: main}}}}\noutputs: {app: {target: native, type: executable, " + field + "}}");
        quxlang::source_bundle bundle;
        EXPECT_THROW(quxlang::detail::parse_build_configuration(config, bundle, std::nullopt), quxlang::compilation_error);
    }
}
