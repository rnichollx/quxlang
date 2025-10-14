// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com
// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/source_loader.hpp"

#include "rpnx/debug.hpp"
#include "rpnx/value.hpp"

#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>
namespace quxlang
{
    source_bundle load_bundle_sources_for_targets(std::filesystem::path const& path, std::optional< std::set< std::string > > configured_targets)
    {
        source_bundle output;
        auto input_build = path / "quxbuild.yaml";
        if (!std::filesystem::exists(input_build))
        {
            throw std::logic_error("quxbuild.yaml not found in input directory");
        }
#ifdef WIN32
        auto build_config = YAML::LoadFile(input_build.string());
#else
        auto build_config = YAML::LoadFile(input_build);
#endif

        auto modules_path = path / "modules";

        auto modules_iter = std::filesystem::directory_iterator(modules_path);
        for (auto const& module_dirent : modules_iter)
        {
            auto module_name = module_dirent.path().filename().string();

            // TODO: Check if module_name is valid

            QUXLANG_DEBUG({ std::cout << "Module: " << module_name << std::endl; });

            if (module_name.starts_with('.') || module_name.starts_with('_'))
            {
                continue;
            }

            if (!module_dirent.is_directory())
            {
                throw std::logic_error("Module " + module_name + " is not a directory");
            }

            module_source mod;

            for (auto const& module_file : std::filesystem::recursive_directory_iterator(module_dirent.path() / "sources"))
            {
                QUXLANG_DEBUG({ std::cout << "File: " << module_file.path().string() << std::endl; });

                if (module_name.starts_with("."))
                {
                    // Skip these files
                    continue;
                }

                auto relpath = module_file.path().lexically_relative(path);

                QUXLANG_DEBUG({ std::cout << "Relpath: " << relpath.string() << std::endl; });

                mod.files[relpath.string()] = source_file();

                std::ifstream file(module_file.path(), std::ios::binary | std::ios::in);
                std::string file_contents = std::string(std::istreambuf_iterator< char >(file), std::istreambuf_iterator< char >());

                mod.files[relpath.string()].edit().contents = file_contents;
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

            QUXLANG_DEBUG({ std::cout << "Loading Target: " << target_name << std::endl; });

            auto target_config_node = target_node.second;

            // Validate target-level keys
            {
                static const std::set<std::string> allowed_target_keys = {"platform", "cpu", "outputs", "modules"};
                for (auto const& kv : target_config_node)
                {
                    auto key = kv.first.as<std::string>();
                    if (allowed_target_keys.count(key) == 0)
                    {
                        throw std::logic_error("Unknown field in target '" + target_name + "': " + key);
                    }
                }
            }

            auto platform = target_config_node["platform"].as< std::string >();

            if (platform != "jvm")
            {
                auto cpu = target_config_node["cpu"].as< std::string >();

                quxlang::output_info info;

                if (platform == "linux")
                {
                    info.os_type = quxlang::os::linux;
                    info.binary_type = quxlang::binary::elf;
                }
                else if (platform == "windows")
                {
                    info.os_type = quxlang::os::windows;
                    info.binary_type = quxlang::binary::pe;
                }
                else if (platform == "macos")
                {
                    info.os_type = quxlang::os::macos;
                    info.binary_type = quxlang::binary::macho;
                }
                else
                {
                    throw std::logic_error("Unknown/unsupported platform " + platform);
                    rpnx::unimplemented();
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
                    throw std::logic_error("Unknown/unsupported cpu " + cpu);
                    rpnx::unimplemented();
                }

                target_configuration target_output;
                target_output.target_output_config = info;

                output.targets[target_name] = target_output;

                for (auto const& output : target_config_node["outputs"])
                {
                    std::string output_name = output.first.as< std::string >();
                    auto output_config_node = output.second;
                    output_config v_output_config;

                    // Validate output-level keys
                    {
                        static const std::set<std::string> allowed_output_keys = {"type", "module", "main_functanoid"};
                        for (auto const& kv : output_config_node)
                        {
                            auto key = kv.first.as<std::string>();
                            if (allowed_output_keys.count(key) == 0)
                            {
                                throw std::logic_error("Unknown field in target '" + target_name + "' output '" + output_name + "': " + key);
                            }
                        }
                    }

                    auto output_type_str = output_config_node["type"].as< std::string >();
                    if (output_type_str == "executable")
                    {
                        v_output_config.type = quxlang::output_kind::executable;
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
                        v_output_config.main_functanoid = output_config_node["main_functanoid"].as< std::string >();
                    }


                    target_output.outputs[output_name] = v_output_config;
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
                                throw std::logic_error("Unknown field in target '" + target_name + "' module '" + module_name + "': " + key);
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