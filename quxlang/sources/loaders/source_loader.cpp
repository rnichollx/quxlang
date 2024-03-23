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
            throw std::runtime_error("quxbuild.yaml not found in input directory");
        }

        auto build_config = YAML::LoadFile(input_build);

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
                throw std::runtime_error("Module " + module_name + " is not a directory");
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

                mod.files[relpath] = std::make_shared< source_file >();

                std::ifstream file(module_file.path(), std::ios::binary | std::ios::in);
                std::string file_contents = std::string(std::istreambuf_iterator< char >(file), std::istreambuf_iterator< char >());

                mod.files[relpath]->contents = file_contents;
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

            auto platform = target_node.second["platform"].as< std::string >();

            if (platform != "jvm")
            {
                auto cpu = target_node.second["cpu"].as< std::string >();

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
                    throw std::runtime_error("Unknown/unsupported platform " + platform);
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
                    throw std::runtime_error("Unknown/unsupported cpu " + cpu);
                    rpnx::unimplemented();
                }

                target_configuration target_output;
                target_output.target_output_config = info;

                output.targets[target_name] = target_output;

                for (auto const& output : target_node.second["outputs"])
                {
                    std::string output_name = output.first.as< std::string >();
                    auto output_config_node = output.second;
                    output_config v_output_config;

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



                for (auto const & module_pair: target_node.second["modules"])
                {
                    module_configuration mod;
                    auto module_name = module_pair.first.as< std::string >();

                    auto module_node = module_pair.second;

                    if (module_node["source"].IsDefined())
                    {
                        mod.source = module_node["source"].as< std::string >();
                    }
                    else
                    {
                        mod.source = module_name;
                    }

                    target_output.module_configurations[module_name] = mod;

                }

                output.targets[target_name] = target_output;



            }
            else
            {
                rpnx::unimplemented();
            }
        }

        return output;
    }
} // namespace quxlang