// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/source_loader.hpp"
#include <quxlang/cpu_attributes.hpp>
#include <quxlang/data/compilation_result.hpp>
#include <quxlang/exception.hpp>

#include "source_loader_internal.hpp"

#include "quxlang/macros.hpp"

#include "rpnx/unimplemented.hpp"

#include <fstream>
#include <iostream>
#include <set>
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
        case '*':
            return true;
        default:
            return false;
        }
    }

    void validate_filename_component(std::string_view relative_path, std::string_view component)
    {
        if (component.size() > portable_filename_byte_limit)
        {
            throw quxlang::reproducibility_error("Source bundle path '" + std::string(relative_path) + "' has a filename longer than 255 bytes");
        }

        for (unsigned char const character : component)
        {
            if (is_illegal_portable_filename_byte(character))
            {
                throw quxlang::reproducibility_error("Source bundle path '" + std::string(relative_path) + "' contains a filename character which is not portable across major filesystems");
            }
        }

        if (!component.empty() && (component.back() == ' ' || component.back() == '.'))
        {
            throw quxlang::reproducibility_error("Source bundle path '" + std::string(relative_path) + "' has a filename ending in a space or period, which is not portable across major filesystems");
        }
    }

    auto contains_byte_order_mark(std::string_view contents) -> bool
    {
        constexpr std::string_view utf8_bom = "\xef\xbb\xbf";
        constexpr std::string_view utf16_big_endian_bom = "\xfe\xff";
        constexpr std::string_view utf16_little_endian_bom = "\xff\xfe";

        return contents.find(utf8_bom) != std::string_view::npos || contents.find(utf16_big_endian_bom) != std::string_view::npos || contents.find(utf16_little_endian_bom) != std::string_view::npos;
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

    /** Validates and records one attribute constraint for a CPU stepping. */
    void insert_cpu_stepping_attribute(cpu_stepping_configuration& stepping, std::string const& attribute_name, bool enabled, cpu target_cpu, std::size_t stepping_index, std::string const& stepping_context)
    {
        std::optional< std::pair< cpu, std::string > > const parsed_attribute = parse_cpu_attribute_stem(attribute_name);
        if (!parsed_attribute.has_value())
        {
            throw quxlang::semantic_compilation_error("Unknown or experimental CPU attribute in " + stepping_context + ": " + attribute_name);
        }
        if (parsed_attribute->first != target_cpu)
        {
            throw quxlang::semantic_compilation_error("CPU attribute in " + stepping_context + " does not apply to the target CPU: " + attribute_name);
        }
        if (stepping_index == 0 && !enabled)
        {
            throw quxlang::semantic_compilation_error("Stepping 0 cannot reject CPU attribute " + attribute_name);
        }

        std::pair< std::map< std::string, bool >::iterator, bool > const insertion = stepping.attributes.emplace(attribute_name, enabled);
        if (!insertion.second)
        {
            throw quxlang::semantic_compilation_error("Duplicate CPU attribute in " + stepping_context + ": " + insertion.first->first);
        }
    }

    auto parse_cpu_stepping_configurations(YAML::Node const& node, cpu target_cpu, std::string const& context) -> std::vector< cpu_stepping_configuration >
    {
        if (!node.IsSequence())
        {
            throw quxlang::semantic_compilation_error(context + " steppings must be a sequence");
        }
        if (node.size() == 0)
        {
            throw quxlang::semantic_compilation_error(context + " steppings must contain at least stepping 0");
        }

        std::vector< cpu_stepping_configuration > output;
        output.reserve(node.size());
        for (std::size_t stepping_index = 0; stepping_index < node.size(); ++stepping_index)
        {
            YAML::Node const stepping_node = node[stepping_index];
            std::string const stepping_context = context + " stepping " + std::to_string(stepping_index);
            if (!stepping_node.IsMap())
            {
                throw quxlang::semantic_compilation_error(stepping_context + " must be an object");
            }

            for (YAML::const_iterator field = stepping_node.begin(); field != stepping_node.end(); ++field)
            {
                std::string const key = field->first.as< std::string >();
                if (key != "attributes" && key != "tune")
                {
                    throw quxlang::semantic_compilation_error("Unknown field in " + stepping_context + ": " + key);
                }
            }

            YAML::Node const attributes_node = stepping_node["attributes"];
            if (!attributes_node.IsDefined())
            {
                throw quxlang::semantic_compilation_error(stepping_context + " must define attributes");
            }

            cpu_stepping_configuration stepping;
            YAML::Node const tune_node = stepping_node["tune"];
            if (tune_node.IsDefined())
            {
                std::string tune = tune_node.as< std::string >();
                std::optional< cpu_tuning_model_entry > const parsed_tune = parse_cpu_tuning_model(tune);
                if (!parsed_tune.has_value())
                {
                    throw quxlang::semantic_compilation_error("Unknown CPU tuning model in " + stepping_context + ": " + tune);
                }
                if (parsed_tune->cpu_type != target_cpu)
                {
                    throw quxlang::semantic_compilation_error("CPU tuning model in " + stepping_context + " does not apply to the target CPU: " + tune);
                }
                stepping.tune = std::move(tune);
            }
            if (attributes_node.IsSequence())
            {
                for (YAML::Node const& attribute_node : attributes_node)
                {
                    insert_cpu_stepping_attribute(stepping, attribute_node.as< std::string >(), true, target_cpu, stepping_index, stepping_context);
                }
            }
            else if (attributes_node.IsMap())
            {
                for (YAML::const_iterator attribute = attributes_node.begin(); attribute != attributes_node.end(); ++attribute)
                {
                    insert_cpu_stepping_attribute(stepping, attribute->first.as< std::string >(), attribute->second.as< bool >(), target_cpu, stepping_index, stepping_context);
                }
            }
            else
            {
                throw quxlang::semantic_compilation_error(stepping_context + " attributes must be a sequence or boolean object");
            }

            output.push_back(std::move(stepping));
        }

        return output;
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
            throw reproducibility_error("Source bundle paths '" + existing->second + "' and '" + generic_relative_path + "' differ only in capitalization");
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
                static const std::set< std::string > allowed_target_keys = {"platform", "cpu", "binary", "environment", "backend", "backend_llvm_options", "backend_cortado_options", "unimplemented_mode", "run_static_tests", "steppings", "outputs", "modules"};
                for (YAML::const_iterator iterator = target_config_node.begin(); iterator != target_config_node.end(); ++iterator)
                {
                    std::string const key = iterator->first.as< std::string >();
                    if (allowed_target_keys.count(key) == 0)
                    {
                        throw quxlang::semantic_compilation_error("Unknown field in target '" + target_name + "': " + key);
                    }
                }
            }

            std::optional< std::string > platform;
            if (target_config_node["platform"].IsDefined())
            {
                platform = target_config_node["platform"].as< std::string >();
            }
            std::optional< std::string > configured_cpu;
            if (target_config_node["cpu"].IsDefined())
            {
                configured_cpu = target_config_node["cpu"].as< std::string >();
            }

            bool const platform_is_jvm = platform.has_value() && *platform == "jvm";
            bool const cpu_is_jvm = configured_cpu.has_value() && *configured_cpu == "jvm";
            if (platform_is_jvm != cpu_is_jvm && platform.has_value() && configured_cpu.has_value())
            {
                throw quxlang::semantic_compilation_error("Target '" + target_name + "' mixes JVM and native platform/CPU settings");
            }
            bool const target_is_jvm = platform_is_jvm || cpu_is_jvm;

                quxlang::machine_target_info info;
            if (target_is_jvm)
            {
                info.cpu_type = quxlang::cpu::jvm;
                if (target_config_node["binary"].IsDefined() || target_config_node["environment"].IsDefined())
                {
                    throw quxlang::semantic_compilation_error("JVM target '" + target_name + "' cannot configure binary or environment");
                }
            }
            else
            {
                if (!platform.has_value())
                {
                    throw quxlang::semantic_compilation_error("Native target '" + target_name + "' requires platform");
                }
                if (!configured_cpu.has_value())
                {
                    throw quxlang::semantic_compilation_error("Native target '" + target_name + "' requires cpu");
                }

                if (*platform == "linux")
                {
                    info.os_type = quxlang::os::linux;
                    info.binary_type = quxlang::binary::elf;
                    info.environment_type = quxlang::environment::static_;
                }
                else if (*platform == "windows")
                {
                    info.os_type = quxlang::os::windows;
                    info.binary_type = quxlang::binary::pe;
                    info.environment_type = quxlang::environment::msvc;
                }
                else if (*platform == "macos")
                {
                    info.os_type = quxlang::os::macos;
                    info.binary_type = quxlang::binary::macho;
                    info.environment_type = quxlang::environment::libsystem;
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Unknown/unsupported platform " + *platform);
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

                if (*configured_cpu == "x64")
                {
                    info.cpu_type = quxlang::cpu::x86_64;
                }
                else if (*configured_cpu == "x86")
                {
                    info.cpu_type = quxlang::cpu::x86_32;
                }
                else if (*configured_cpu == "ARM32")
                {
                    info.cpu_type = quxlang::cpu::arm_32;
                }
                else if (*configured_cpu == "ARM64")
                {
                    info.cpu_type = quxlang::cpu::arm_64;
                }
                else if (*configured_cpu == "z_arch")
                {
                    info.cpu_type = quxlang::cpu::z_arch;
                }
                else
                {
                    throw quxlang::semantic_compilation_error("Unknown/unsupported cpu " + *configured_cpu);
                }
            }

            target_configuration target_output;
            target_output.target_output_config = info;
            target_output.backend = target_is_jvm ? quxlang::backend_kind::cortado : quxlang::backend_kind::llvm;

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

            auto parse_backend_cortado_options = [](YAML::Node const& backend_cortado_options_node, std::string const& context) -> quxlang::backend_cortado_options
            {
                quxlang::backend_cortado_options output;
                static const std::set< std::string > allowed_cortado_option_keys = {"mode"};
                for (YAML::const_iterator iterator = backend_cortado_options_node.begin(); iterator != backend_cortado_options_node.end(); ++iterator)
                {
                    std::string const key = iterator->first.as< std::string >();
                    if (allowed_cortado_option_keys.count(key) == 0)
                    {
                        throw quxlang::semantic_compilation_error("Unknown field in " + context + " backend_cortado_options: " + key);
                    }
                }

                if (backend_cortado_options_node["mode"].IsDefined())
                {
                    std::string const mode = backend_cortado_options_node["mode"].as< std::string >();
                    if (mode == "standard")
                    {
                        output.mode = quxlang::backend_cortado_mode::standard;
                    }
                    else if (mode == "address_sanitizer")
                    {
                        output.mode = quxlang::backend_cortado_mode::address_sanitizer;
                    }
                    else
                    {
                        throw quxlang::semantic_compilation_error("Unknown/unsupported Cortado backend mode " + mode);
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
                else if (backend == "cortado")
                {
                    target_output.backend = quxlang::backend_kind::cortado;
                }
                    else
                    {
                        throw quxlang::semantic_compilation_error("Unknown/unsupported backend " + backend);
                    }
                }

            if (target_is_jvm && target_output.backend != quxlang::backend_kind::cortado)
            {
                throw quxlang::semantic_compilation_error("Target '" + target_name + "' LLVM backend cannot target JVM");
            }
            if (!target_is_jvm && target_output.backend == quxlang::backend_kind::cortado)
            {
                throw quxlang::semantic_compilation_error("Target '" + target_name + "' Cortado backend requires JVM");
            }
                if (target_config_node["backend_llvm_options"].IsDefined())
                {
                if (target_output.backend != quxlang::backend_kind::llvm)
                {
                    throw quxlang::semantic_compilation_error("Target '" + target_name + "' configures LLVM options for a non-LLVM backend");
                }
                    target_output.llvm_options = parse_backend_llvm_options(target_config_node["backend_llvm_options"], "target '" + target_name + "'");
                }
            if (target_config_node["backend_cortado_options"].IsDefined())
            {
                if (target_output.backend != quxlang::backend_kind::cortado)
                {
                    throw quxlang::semantic_compilation_error("Target '" + target_name + "' configures Cortado options for a non-Cortado backend");
                }
                target_output.cortado_options = parse_backend_cortado_options(target_config_node["backend_cortado_options"], "target '" + target_name + "'");
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
                if (target_config_node["steppings"].IsDefined())
                {
                if (target_is_jvm)
                {
                    throw quxlang::semantic_compilation_error("JVM target '" + target_name + "' cannot configure CPU steppings");
                }
                target_output.steppings = detail::parse_cpu_stepping_configurations(target_config_node["steppings"], info.cpu_type, "Target '" + target_name + "'");
                }

                if (target_config_node["outputs"].IsDefined())
                {
                    target_output.outputs = std::map< std::string, output_config >{};
                YAML::Node const configured_outputs = target_config_node["outputs"];
                for (YAML::const_iterator iterator = configured_outputs.begin(); iterator != configured_outputs.end(); ++iterator)
                    {
                    std::string output_name = iterator->first.as< std::string >();
                    YAML::Node output_config_node = iterator->second;
                        output_config v_output_config;

                    static const std::set< std::string > allowed_output_keys = {"type", "modules", "main_functanoid", "backend_llvm_options", "backend_cortado_options"};
                    for (YAML::const_iterator output_iterator = output_config_node.begin(); output_iterator != output_config_node.end(); ++output_iterator)
                            {
                        std::string const key = output_iterator->first.as< std::string >();
                                if (allowed_output_keys.count(key) == 0)
                                {
                                    throw quxlang::semantic_compilation_error("Unknown field in target '" + target_name + "' output '" + output_name + "': " + key);
                                }
                            }

                    std::string const output_type = output_config_node["type"].as< std::string >();
                    if (output_type == "executable")
                        {
                            v_output_config.type = quxlang::output_kind::executable;
                        }
                    else if (output_type == "shared_library")
                    {
                        v_output_config.type = quxlang::output_kind::shared_library;
                    }
                    else if (output_type == "static_library")
                    {
                        v_output_config.type = quxlang::output_kind::static_library;
                    }
                    else if (output_type == "image")
                    {
                        v_output_config.type = quxlang::output_kind::image;
                    }
                    else if (output_type == "unit_test_suite")
                        {
                            v_output_config.type = quxlang::output_kind::unit_test_suite;
                        }
                        else
                        {
                        throw quxlang::semantic_compilation_error("Unknown/unsupported output type " + output_type);
                        }

                        if (output_config_node["modules"].IsDefined())
                        {
                            YAML::Node const modules_node = output_config_node["modules"];
                            output_module_selection selection;
                            if (modules_node.IsScalar())
                            {
                                std::string const scalar_selection = modules_node.as< std::string >();
                                if (scalar_selection != "ALL")
                                {
                                    throw quxlang::semantic_compilation_error("Output '" + output_name + "' modules must be ALL or an array of module names");
                                }
                                selection.all_modules = true;
                            }
                            else if (modules_node.IsSequence())
                            {
                                std::set< std::string > unique_module_names;
                                for (YAML::const_iterator module_iterator = modules_node.begin(); module_iterator != modules_node.end(); ++module_iterator)
                                {
                                    if (!module_iterator->IsScalar())
                                    {
                                        throw quxlang::semantic_compilation_error("Output '" + output_name + "' modules array entries must be module names");
                                    }
                                    std::string const module_name = module_iterator->as< std::string >();
                                    if (!unique_module_names.insert(module_name).second)
                                    {
                                        throw quxlang::semantic_compilation_error("Output '" + output_name + "' lists module '" + module_name + "' more than once");
                                    }
                                    selection.module_names.push_back(module_name);
                                }
                                if (selection.module_names.empty())
                                {
                                    throw quxlang::semantic_compilation_error("Output '" + output_name + "' modules array cannot be empty");
                                }
                            }
                            else
                            {
                                throw quxlang::semantic_compilation_error("Output '" + output_name + "' modules must be ALL or an array of module names");
                            }
                            v_output_config.modules = std::move(selection);
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
                        if (target_output.backend != quxlang::backend_kind::llvm)
                        {
                            throw quxlang::semantic_compilation_error("Output '" + output_name + "' configures LLVM options for a non-LLVM backend");
                        }
                        v_output_config.llvm_options = parse_backend_llvm_options(output_config_node["backend_llvm_options"], "target '" + target_name + "' output '" + output_name + "'");
                    }
                    if (output_config_node["backend_cortado_options"].IsDefined())
                    {
                        if (target_output.backend != quxlang::backend_kind::cortado)
                        {
                            throw quxlang::semantic_compilation_error("Output '" + output_name + "' configures Cortado options for a non-Cortado backend");
                        }
                        v_output_config.cortado_options = parse_backend_cortado_options(output_config_node["backend_cortado_options"], "target '" + target_name + "' output '" + output_name + "'");
                    }

                    target_output.outputs->insert_or_assign(output_name, std::move(v_output_config));
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

                }

            output.targets[target_name] = std::move(target_output);
        }

        return output;
    }
} // namespace quxlang
