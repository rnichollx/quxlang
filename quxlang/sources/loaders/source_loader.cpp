// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com


#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>
#include "quxlang/source_loader.hpp"

#include "source_loader_internal.hpp"

#include "quxlang/macros.hpp"

#include "rpnx/unimplemented.hpp"

#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace quxlang::detail
{
    constexpr std::size_t portable_filename_byte_limit = 255;

    auto portable_case_fold(std::string_view value) -> std::string
    {
        std::string output;
        output.reserve(value.size());
        for (unsigned char const character : value)
        {
            if (character >= 'A' && character <= 'Z')
            {
                output.push_back(static_cast< char >(character + ('a' - 'A')));
            }
            else
            {
                output.push_back(character);
            }
        }
        return output;
    }

    auto is_illegal_portable_filename_byte(unsigned char character) -> bool
    {
        if (character < 32)
        {
            return true;
        }

        switch (character)
        {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*': return true;
        default: return false;
        }
    }

    void validate_filename_component(std::string_view relative_path, std::string_view component)
    {
        if (component.size() > portable_filename_byte_limit)
        {
            throw quxlang::reproducibility_error(
                "Source bundle path '" + std::string(relative_path) + "' has a filename longer than 255 bytes");
        }

        for (unsigned char const character : component)
        {
            if (is_illegal_portable_filename_byte(character))
            {
                throw quxlang::reproducibility_error(
                    "Source bundle path '" + std::string(relative_path) + "' contains a filename character which is not portable across major filesystems");
            }
        }

        if (!component.empty() && (component.back() == ' ' || component.back() == '.'))
        {
            throw quxlang::reproducibility_error(
                "Source bundle path '" + std::string(relative_path) + "' has a filename ending in a space or period, which is not portable across major filesystems");
        }
    }

    auto contains_byte_order_mark(std::string_view contents) -> bool
    {
        constexpr std::string_view utf8_bom = "\xef\xbb\xbf";
        constexpr std::string_view utf16_big_endian_bom = "\xfe\xff";
        constexpr std::string_view utf16_little_endian_bom = "\xff\xfe";

        return contents.find(utf8_bom) != std::string_view::npos ||
               contents.find(utf16_big_endian_bom) != std::string_view::npos ||
               contents.find(utf16_little_endian_bom) != std::string_view::npos;
    }
} // namespace quxlang::detail

namespace quxlang::detail
{
    /**
     * Parses a quxbuild target binary value into the internal binary type enum.
     */
    auto parse_binary_type(std::string const& binary) -> quxlang::binary
    {
        if (binary == "elf")
        {
            return binary::elf;
        }
        if (binary == "macho")
        {
            return binary::macho;
        }
        if (binary == "pe")
        {
            return binary::pe;
        }
        if (binary == "wasm")
        {
            return binary::wasm;
        }

        throw quxlang::semantic_compilation_error("Unknown/unsupported binary " + binary);
    }

    /**
     * Parses a quxbuild target environment value into the internal environment type enum.
     */
    auto parse_environment_type(std::string const& environment) -> quxlang::environment
    {
        if (environment == "glibc")
        {
            return quxlang::environment::glibc;
        }
        if (environment == "musl")
        {
            return quxlang::environment::musl;
        }
        if (environment == "bionic")
        {
            return quxlang::environment::bionic;
        }
        if (environment == "msvc")
        {
            return quxlang::environment::msvc;
        }
        if (environment == "ucrt")
        {
            return quxlang::environment::ucrt;
        }
        if (environment == "cygwin")
        {
            return quxlang::environment::cygwin;
        }
        if (environment == "static")
        {
            return quxlang::environment::static_;
        }
        if (environment == "libsystem")
        {
            return quxlang::environment::libsystem;
        }
        if (environment == "freestanding")
        {
            return quxlang::environment::freestanding;
        }

        throw quxlang::semantic_compilation_error("Unknown/unsupported environment " + environment);
    }
} // namespace quxlang::detail

namespace quxlang::detail
{
    void source_path_validator::add(std::filesystem::path const& relative_path)
    {
        std::string const generic_relative_path = relative_path.generic_string();
        for (std::filesystem::path const& component_path : relative_path)
        {
            std::string const component = component_path.generic_string();
            if (component == "." || component == "..")
            {
                throw reproducibility_error("Source bundle path '" + generic_relative_path + "' is not a normalized relative path");
            }
            validate_filename_component(generic_relative_path, component);
        }

        std::string const folded_path = portable_case_fold(generic_relative_path);
        auto const [existing, inserted] = m_case_folded_paths.emplace(folded_path, generic_relative_path);
        if (!inserted && existing->second != generic_relative_path)
        {
            throw reproducibility_error(
                "Source bundle paths '" + existing->second + "' and '" + generic_relative_path + "' differ only in capitalization");
        }
    }

    void validate_source_file_contents(std::string_view relative_path, std::string_view contents)
    {
        if (contents.find('\r') != std::string_view::npos)
        {
            throw reproducibility_error("Source file '" + std::string(relative_path) + "' contains a carriage return");
        }
        if (contains_byte_order_mark(contents))
        {
            throw reproducibility_error("Source file '" + std::string(relative_path) + "' contains a byte-order mark");
        }
    }
} // namespace quxlang::detail

namespace quxlang
{
    source_bundle load_bundle_sources_for_targets(std::filesystem::path const& path, std::optional< std::set< std::string > > configured_targets)
    {
        source_bundle output;
        auto input_build = path / "quxbuild.yaml";
        if (!std::filesystem::exists(input_build))
        {
            throw quxlang::semantic_compilation_error("quxbuild.yaml not found in input directory");
        }
#ifdef WIN32
        auto build_config = YAML::LoadFile(input_build.string());
#else
        auto build_config = YAML::LoadFile(input_build);
#endif

        auto modules_path = path / "modules";

        detail::source_path_validator source_path_validator;

        auto modules_iter = std::filesystem::directory_iterator(modules_path);
        for (auto const& module_dirent : modules_iter)
        {
            auto module_name = module_dirent.path().filename().string();

            // TODO: Check if module_name is valid

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "Module: " << module_name << std::endl;
            }

            if (module_name.starts_with('.') || module_name.starts_with('_'))
            {
                continue;
            }

            source_path_validator.add(module_dirent.path().lexically_relative(path));

            if (!module_dirent.is_directory())
            {
                throw quxlang::semantic_compilation_error("Module " + module_name + " is not a directory");
            }

            module_source mod;

            auto module_sources_path = module_dirent.path() / "sources";
            if (!std::filesystem::is_directory(module_sources_path))
            {
                throw quxlang::semantic_compilation_error("Module " + module_name + " does not have a sources directory");
            }

            for (auto const& module_file : std::filesystem::recursive_directory_iterator(module_sources_path))
            {
                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    std::cout << "File: " << module_file.path().string() << std::endl;
                }

                std::string const relpath = module_file.path().lexically_relative(path).generic_string();

                source_path_validator.add(module_file.path().lexically_relative(path));

                if (module_file.is_directory())
                {
                    continue;
                }

                if (!module_file.is_regular_file())
                {
                    throw reproducibility_error("Source bundle path '" + relpath + "' is not a regular file or directory");
                }

                if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
                {
                    std::cout << "Relpath: " << relpath << std::endl;
                }

                std::ifstream file(module_file.path(), std::ios::binary | std::ios::in);
                if (!file)
                {
                    throw reproducibility_error("Source file '" + relpath + "' could not be opened");
                }
                std::string file_contents = std::string(std::istreambuf_iterator< char >(file), std::istreambuf_iterator< char >());

                detail::validate_source_file_contents(relpath, file_contents);

                mod.files[relpath].edit().contents = std::move(file_contents);
            }

            output.module_sources[module_name] = mod;
        }

        for (auto const target_node : build_config)
        {

            std::string target_name = target_node.first.as< std::string >();
            // If we only want to build certain targets, skip the rest
            if (configured_targets.has_value() && configured_targets->count(target_name) == 0)
            {
                continue;
            }

            if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
            {
                std::cout << "Loading Target: " << target_name << std::endl;
            }

            auto target_config_node = target_node.second;

            // Validate target-level keys
            {
                static const std::set<std::string> allowed_target_keys = {
                    "platform", "cpu", "binary", "environment", "backend", "backend_llvm_options", "unimplemented_mode", "run_static_tests", "outputs", "modules"};
                for (auto const& kv : target_config_node)
                {
                    auto key = kv.first.as<std::string>();
                    if (allowed_target_keys.count(key) == 0)
                    {
                        throw quxlang::semantic_compilation_error("Unknown field in target '" + target_name + "': " + key);
                    }
                }
            }

            auto platform = target_config_node["platform"].as< std::string >();

            if (platform != "jvm")
            {
                auto cpu = target_config_node["cpu"].as< std::string >();

                quxlang::machine_target_info info;

                if (platform == "linux")
                {
                    info.os_type = quxlang::os::linux;
                    info.binary_type = quxlang::binary::elf;
                    info.environment_type = quxlang::environment::static_;
                }
                else if (platform == "windows")
                {
                    info.os_type = quxlang::os::windows;
                    info.binary_type = quxlang::binary::pe;
                    info.environment_type = quxlang::environment::msvc;
                }
                else if (platform == "macos")
                {
                    info.os_type = quxlang::os::macos;
                    info.binary_type = quxlang::binary::macho;
                    info.environment_type = quxlang::environment::libsystem;
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Unknown/unsupported platform " + platform);
                    rpnx::unimplemented();
                }

                if (target_config_node["binary"].IsDefined())
                {
                    std::string const binary = target_config_node["binary"].as< std::string >();
                    info.binary_type = detail::parse_binary_type(binary);
                }

                if (target_config_node["environment"].IsDefined())
                {
                    std::string const environment = target_config_node["environment"].as< std::string >();
                    info.environment_type = detail::parse_environment_type(environment);
                }

                if (cpu == "x64")
                {
                    info.cpu_type = quxlang::cpu::x86_64;
                }
                else if (cpu == "x86")
                {
                    info.cpu_type = quxlang::cpu::x86_32;
                }
                else if (cpu == "ARM32")
                {
                    info.cpu_type = quxlang::cpu::arm_32;
                }
                else if (cpu == "ARM64")
                {
                    info.cpu_type = quxlang::cpu::arm_64;
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Unknown/unsupported cpu " + cpu);
                    rpnx::unimplemented();
                }

                target_configuration target_output;
                target_output.target_output_config = info;

                auto parse_backend_llvm_options = [](YAML::Node const& backend_llvm_options_node, std::string const& context) -> quxlang::backend_llvm_options
                {
                    quxlang::backend_llvm_options output;
                    static const std::set<std::string> allowed_llvm_option_keys = {"mode"};
                    for (auto const& kv : backend_llvm_options_node)
                    {
                        std::string const key = kv.first.as<std::string>();
                        if (allowed_llvm_option_keys.count(key) == 0)
                        {
                            throw quxlang::semantic_compilation_error("Unknown field in " + context + " backend_llvm_options: " + key);
                        }
                    }

                    if (backend_llvm_options_node["mode"].IsDefined())
                    {
                        std::string const mode = backend_llvm_options_node["mode"].as< std::string >();
                        if (mode == "optimize")
                        {
                            output.mode = quxlang::backend_llvm_mode::optimize;
                        }
                        else if (mode == "debug")
                        {
                            output.mode = quxlang::backend_llvm_mode::debug;
                        }
                        else
                        {
                            throw quxlang::semantic_compilation_error("Unknown/unsupported LLVM backend mode " + mode);
                        }
                    }

                    return output;
                };

                if (target_config_node["backend"].IsDefined())
                {
                    std::string const backend = target_config_node["backend"].as< std::string >();
                    if (backend == "llvm")
                    {
                        target_output.backend = quxlang::backend_kind::llvm;
                    }
                    else
                    {
                        throw quxlang::semantic_compilation_error("Unknown/unsupported backend " + backend);
                    }
                }

                if (target_config_node["backend_llvm_options"].IsDefined())
                {
                    target_output.llvm_options = parse_backend_llvm_options(target_config_node["backend_llvm_options"], "target '" + target_name + "'");
                }

                if (target_config_node["unimplemented_mode"].IsDefined())
                {
                    std::string const mode = target_config_node["unimplemented_mode"].as< std::string >();
                    if (mode == "trap")
                    {
                        target_output.unimplemented_mode = quxlang::unimplemented_mode::trap;
                    }
                    else if (mode == "error")
                    {
                        target_output.unimplemented_mode = quxlang::unimplemented_mode::error;
                    }
                    else
                    {
                        throw quxlang::semantic_compilation_error("Unknown/unsupported unimplemented_mode " + mode);
                    }
                }

                if (target_config_node["run_static_tests"].IsDefined())
                {
                    target_output.run_static_tests = target_config_node["run_static_tests"].as< bool >();
                }

                output.targets[target_name] = target_output;

                if (target_config_node["outputs"].IsDefined())
                {
                    target_output.outputs = std::map< std::string, output_config >{};
                    for (auto const& output : target_config_node["outputs"])
                    {
                        std::string output_name = output.first.as< std::string >();
                        auto output_config_node = output.second;
                        output_config v_output_config;

                        // Validate output-level keys
                        {
                            static const std::set<std::string> allowed_output_keys = {"type", "module", "main_functanoid", "backend_llvm_options"};
                            for (auto const& kv : output_config_node)
                            {
                                auto key = kv.first.as<std::string>();
                                if (allowed_output_keys.count(key) == 0)
                                {
                                    throw quxlang::semantic_compilation_error("Unknown field in target '" + target_name + "' output '" + output_name + "': " + key);
                                }
                            }
                        }

                        auto output_type_str = output_config_node["type"].as< std::string >();
                        if (output_type_str == "executable")
                        {
                            v_output_config.type = quxlang::output_kind::executable;
                        }
                        else if (output_type_str == "unit_test_suite")
                        {
                            v_output_config.type = quxlang::output_kind::unit_test_suite;
                        }
                        else
                        {
                            rpnx::unimplemented();
                        }

                        if (output_config_node["module"].IsDefined())
                        {
                            v_output_config.module = output_config_node["module"].as< std::string >();
                        }

                        if (output_config_node["main_functanoid"].IsDefined())
                        {
                            if (v_output_config.type == quxlang::output_kind::unit_test_suite)
                            {
                                throw quxlang::semantic_compilation_error("Output '" + output_name + "' of type unit_test_suite cannot configure main_functanoid");
                            }
                            v_output_config.main_functanoid = output_config_node["main_functanoid"].as< std::string >();
                        }

                        if (output_config_node["backend_llvm_options"].IsDefined())
                        {
                            v_output_config.llvm_options =
                                parse_backend_llvm_options(output_config_node["backend_llvm_options"], "target '" + target_name + "' output '" + output_name + "'");
                        }

                        target_output.outputs->insert_or_assign(output_name, v_output_config);
                    }
                }

                for (auto const& module_pair : target_config_node["modules"])
                {
                    module_configuration mod;
                    auto module_name = module_pair.first.as< std::string >();

                    auto module_node = module_pair.second;

                    // Validate module-level keys
                    {
                        static const std::set<std::string> allowed_module_keys = {"source", "options"};
                        for (auto const& kv : module_node)
                        {
                            auto key = kv.first.as<std::string>();
                            if (allowed_module_keys.count(key) == 0)
                            {
                                throw quxlang::semantic_compilation_error("Unknown field in target '" + target_name + "' module '" + module_name + "': " + key);
                            }
                        }
                    }

                    if (module_node["source"].IsDefined())
                    {
                        mod.source = module_node["source"].as< std::string >();
                    }
                    else
                    {
                        mod.source = module_name;
                    }

                    if (module_node["options"].IsDefined())
                    {
                        for (auto const& option_pair : module_node["options"])
                        {
                            auto option_name = option_pair.first;
                            auto option_value_node = option_pair.second;

                            auto option_name_str = option_name.as< std::string >();
                            auto option_value_str = option_value_node.as< std::string >();

                            mod.option_values[option_name_str] = option_value_str;
                        }
                    }

                    // After fully populating the module configuration (including options),
                    // store it into the target configuration map.
                    target_output.module_configurations[module_name] = mod;

                    output.targets[target_name] = target_output;

                    
                }
            }
            else
            {
                rpnx::unimplemented();
            }
        }

        return output;
    }
} // namespace quxlang
