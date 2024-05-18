// Copyright (c) 2024 Ryan Nicholl $USER_EMAIL

#include "quxlang/backends/asm/arm_asm_converter.hpp"
#include "quxlang/backends/llvm/llvm_code_generator.hpp"
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

        quxlang::llvm_code_generator cg(target_config.target_output_config);

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

            auto start_sym = quxlang::parsers::parse_type_symbol("::runtime_main");
            start_sym = quxlang::with_context(start_sym, quxlang::module_reference{main_module_name});

            std::set< quxlang::type_symbol > to_compile_vmir{mainfunc_sym};
            std::set< quxlang::type_symbol > to_compile_asm{start_sym};

            while (to_compile_vmir.size() != 0 || to_compile_asm.size() != 0)
            {
                if (to_compile_vmir.size() != 0)
                {
                    auto sym = *to_compile_vmir.begin();
                    std::cout << "Compiling symbol: " << quxlang::mangle(sym) << std::endl;

                    if (!quxlang::typeis<quxlang::instanciation_reference>(sym))
                    {
                        throw std::runtime_error("Expected instanciation reference");
                    }

                    quxlang::vm_procedure proc = c.get_vm_procedure2(quxlang::as<quxlang::instanciation_reference>(sym));

                    std::string procname = quxlang::mangle(sym);

                    std::vector< std::byte > proc_data;
                    std::vector< std::byte > proc_json;
                    std::vector< std::byte > proc_llvm_bc;
                    std::vector< std::byte > proc_elf;

                    rpnx::serialize_iter(proc, std::back_inserter(proc_data));
                    rpnx::json_serialize_iter(proc, std::back_inserter(proc_json));

                    proc_llvm_bc = cg.qxbc_to_llvm_bc(proc);
                    proc_elf = cg.compile_llvm_ir_to_elf(proc_llvm_bc);

                    std::ofstream outfile(output / target_name / "build" / (procname + ".qxbc"), std::ios::binary | std::ios::trunc);
                    std::ofstream outfile2(output / target_name / "build" / (procname + ".json"), std::ios::binary | std::ios::trunc);
                    std::ofstream outfile3(output / target_name / "build" / (procname + ".bc"), std::ios::binary | std::ios::trunc);
                    std::ofstream outfile4(output / target_name / "build" / (procname + ".o"), std::ios::binary | std::ios::trunc);

                    outfile.write(reinterpret_cast< char const* >(proc_data.data()), proc_data.size());
                    outfile2.write(reinterpret_cast< char const* >(proc_json.data()), proc_json.size());
                    outfile3.write(reinterpret_cast< char const* >(proc_llvm_bc.data()), proc_llvm_bc.size());
                    outfile4.write(reinterpret_cast< char const* >(proc_elf.data()), proc_elf.size());

                    to_compile_vmir.erase(sym);
                    compiled_vmir.insert(sym);

                    outfile.close();
                    outfile2.close();
                    outfile3.close();
                    outfile4.close();

                    for (auto const& sym2 : proc.invoked_functanoids)
                    {
                        std::cout << "Invoked: " << quxlang::mangle(sym2) << std::endl;
                        if (compiled_vmir.find(sym2) == compiled_vmir.end())
                        {
                            std::cout << "should compile: " << quxlang::mangle(sym2) << std::endl;
                            to_compile_vmir.insert(sym2);

                            std::cout << "to_compile_vmir.size() = " << to_compile_vmir.size() << std::endl;
                        }
                        else
                        {
                            std::cout << "already compiled: " << quxlang::mangle(sym2) << std::endl;
                        }
                    }

                    for (auto const& sym2 : proc.invoked_asm_procedures)
                    {
                        std::cout << "Invoked asm: " << quxlang::mangle(sym2) << std::endl;
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

                    std::cout << "Compiling asm symbol: " << quxlang::mangle(sym) << std::endl;

                    quxlang::asm_procedure proc = c.get_asm_procedure_from_canonical_symbol(sym);



                    std::vector< std::byte > proc_data;

                    std::string assembler_string = quxlang::convert_to_arm_asm(proc.instructions.begin(), proc.instructions.end(), proc.name);

                    std::ofstream outfile(output / target_name / "build" / (proc.name + ".s"), std::ios::binary | std::ios::trunc);

                    outfile.write(assembler_string.data(), assembler_string.size());
                    outfile.close();

                    to_compile_asm.erase(sym);
                    compiled_asm.insert(sym);
                }
            }
        }
    }
}