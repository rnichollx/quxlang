// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/compiler.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "quxlang/parsers/parse_type_symbol.hpp"
#include "quxlang/source_loader.hpp"
#include "rpnx/serializer.hpp"
#include "rpnx/value.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <yaml-cpp/yaml.h>

int main(int argc, char** argv)
{
    bool verbose = true;
    if (argc < 3)
    {
        throw std::runtime_error("Usage: qxc <input directory> <output directory> [targets,...]");
    }

    std::filesystem::path input = argv[1];
    std::filesystem::path output = argv[2];

    quxlang::source_bundle input_srcs = quxlang::load_bundle_sources_for_targets(input, std::nullopt);

    for (auto const& [target_name, target_config] : input_srcs.targets)
    {
        if (verbose)
        {
            std::cout << "Compiling Target: " << target_name << std::endl;
        }

        quxlang::compiler c(input_srcs, target_name);

        for (auto const& [output_name, output_config] : target_config.outputs)
        {
            if (verbose)
            {
                std::cout << "Compiling: " << target_name << "/" << output_name << std::endl;
            }

            std::filesystem::create_directories(output / target_name / "build");

            std::set< quxlang::type_symbol > compiled_vmir;
            std::set< quxlang::type_symbol > compiled_asm;

            std::string main_module_name = output_config.module.value_or("::main@()");
            std::string main_function_name = output_config.main_functanoid.value_or("::main@()");

            auto mainfunc_sym = quxlang::parsers::parse_type_symbol(main_function_name);
            mainfunc_sym = quxlang::with_context(mainfunc_sym, quxlang::module_reference{main_module_name});

            std::set< quxlang::type_symbol > to_compile_vmir{mainfunc_sym};
            std::set< quxlang::type_symbol > to_compile_asm;

            while (to_compile_vmir.size() != 0 || to_compile_asm.size() != 0)
            {
                if (to_compile_vmir.size() != 0)
                {
                    auto sym = *to_compile_vmir.begin();

                    quxlang::vm_procedure proc = c.get_vm_procedure_from_canonical_functanoid(mainfunc_sym);

                    std::string procname = quxlang::mangle(mainfunc_sym);

                    std::vector< std::byte > proc_data;
                    std::vector< std::byte > proc_json;

                    rpnx::serialize_iter(proc, std::back_inserter(proc_data));
                    rpnx::json_serialize_iter(proc, std::back_inserter(proc_json));

                    std::ofstream outfile(output / target_name / "build" / (procname + ".qxvmir"), std::ios::binary| std::ios::trunc);
                    std::ofstream outfile2(output / target_name / "build" / (procname + ".json"), std::ios::binary | std::ios::trunc);

                    outfile.write(reinterpret_cast< char const* >(proc_data.data()), proc_data.size());
                    outfile2.write(reinterpret_cast< char const* >(proc_json.data()), proc_json.size());

                    to_compile_vmir.erase(sym);
                    compiled_vmir.insert(sym);

                    for (auto const& sym2 : proc.invoked_functanoids)
                    {
                        if (compiled_vmir.find(sym2) == compiled_vmir.end())
                        {
                            to_compile_vmir.insert(sym2);
                        }
                    }

                    for (auto const& sym2 : proc.invoked_asm_procedures)
                    {
                        if (compiled_asm.find(sym2) == compiled_asm.end())
                        {
                            to_compile_asm.insert(sym2);
                        }
                    }
                }
                else
                {
                    auto sym = *to_compile_asm.begin();
                    // TODO: compile this
                    to_compile_asm.erase(sym);
                    compiled_asm.insert(sym);
                }
            }
        }
    }
}