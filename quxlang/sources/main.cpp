// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/backends/llvm/llvm_code_generator.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/converters/qual_converters.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include <iostream>

#include "quxlang/data/vm_executable_unit.hpp"

#include "quxlang/manipulators/vmmanip.hpp"

#include <quxlang/parsers/parse_type_symbol.hpp>

int main(int argc, char** argv)
{
    quxlang::output_info target_machine{
        .cpu_type = quxlang::cpu::arm_64,
        .os_type = quxlang::os::linux,
        .binary_type = quxlang::binary::elf,
    };

    quxlang::compiler c(argc, argv, target_machine);

    quxlang::llvm_code_generator cg(target_machine);
    // cg.foo();
    // return 0;

    quxlang::type_symbol cn = quxlang::module_reference{"main"};
    cn = quxlang::subsymbol{cn, "quz"};
    cn = quxlang::subsymbol{std::move(cn), "bif"};
    cn = quxlang::subsymbol{std::move(cn), "box"};
    cn = quxlang::subsymbol{std::move(cn), "buz"};

    quxlang::int_type u32type = quxlang::int_type{32, true};

    quxlang::call_parameter_information args;
    args.argument_types.push_back(u32type);
    args.argument_types.push_back(u32type);

    auto qn = c.get_function_qualname(cn, args);

    std::string name = quxlang::mangle(qn);

    std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();

    std::string boxy_input = "::boxy";
    std::string::iterator boxy_iter = boxy_input.begin();
    auto boxy = quxlang::parsers::parse_type_symbol< std::string::iterator >(boxy_iter, boxy_input.end());

    std::set< quxlang::type_symbol > already_compiled;
    std::set< quxlang::type_symbol > already_assembled;

    std::set< quxlang::type_symbol > new_deps_to_compile = {qn};
    std::set< quxlang::type_symbol > new_deps_to_assemble = {boxy};

    std::map< quxlang::type_symbol, std::vector< std::byte > > compiled_code;

    while (new_deps_to_compile.empty() == false || new_deps_to_assemble.empty() == false)
    {
        if (!new_deps_to_compile.empty())
        {
            auto to_compile = *new_deps_to_compile.begin();

            std::cout << "Compiling " << quxlang::to_string(to_compile) << std::endl;

            auto get_procedure_start = std::chrono::high_resolution_clock::now();
            quxlang::vm_procedure vmf = c.get_vm_procedure_from_canonical_functanoid(to_compile);

            auto get_procedure_duration = std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - get_procedure_start);

            std::cout << "Got procedure " << quxlang::to_string(to_compile) << " in " << std::dec << get_procedure_duration.count() << " microseconds" << std::endl;
            // std::cout << quxlang::to_string(quxlang::vm_executable_unit{vmf.body}) << std::endl;

            auto generate_code_start = std::chrono::high_resolution_clock::now();
            auto llvm_bitcode = cg.qxbc_to_llvm_bc(vmf);
            auto generate_code_duration = std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::high_resolution_clock::now() - generate_code_start);

            std::cout << "Generated LLVM bitcode for " << quxlang::to_string(to_compile) << " in " << std::dec << generate_code_duration.count() << " microseconds" << std::endl;
            compiled_code[to_compile] = llvm_bitcode;
            already_compiled.insert(to_compile);

            for (auto& x : vmf.invoked_functanoids)
            {
                if (already_compiled.contains(x))
                    continue;
                else
                    new_deps_to_compile.insert(x);
            }
            new_deps_to_compile.erase(to_compile);

            for (auto& x : vmf.invoked_asm_procedures)
            {
                if (already_assembled.contains(x))
                    continue;
                else
                    new_deps_to_assemble.insert(x);
            }
        }
        else
        {
            auto to_assemble = *new_deps_to_assemble.begin();

            quxlang::asm_procedure proc = c.get_asm_procedure_from_canonical_symbol(to_assemble);

            auto code = cg.assemble(proc);
            compiled_code[to_assemble] = code;

            new_deps_to_assemble.erase(to_assemble);
        }
    }

    // std::cout << "Got overload:" << name << std::endl;
    //  auto vec = cg.qxbc_to_llvm_bc(quxlang::cpu_arch_armv8a(), func_name );
    /*
        auto files = c.get_file_list();

        auto file_name = files.at(0);

        auto file_contents = c.get_file_contents(file_name);

        std::cout << "File contents of " << file_name << ":\n" << file_contents << "\n";

        // get the AST

        auto ast = c.get_file_ast(file_name);

        std::cout << "AST of " << file_name << ":\n";

        for (auto& cl : ast.root.m_sub_entities)
        {
            std::cout << "Class " << cl.first << ":\n";
            std::cout << cl.second.get().to_string() << "\n";
        }

        // get the class list
        auto list = c.get_class_list();

        for (auto elm : list.class_names)
        {
            std::cout << "Class name: " << elm << "\n";
        }
    */

    std::chrono::time_point< std::chrono::high_resolution_clock > end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast< std::chrono::microseconds >(end_time - start_time);

    std::cout << "Compilation took " << std::dec << duration.count() << " microseconds" << std::endl;

    // return 0;
    quxlang::type_symbol foo = quxlang::initialization_reference{.initializee = quxlang::subsymbol{quxlang::module_reference{.module_name = "main"}, "box2"}, .parameters = {quxlang::parsers::parse_type_symbol("I32")}};

    auto foo_placement_info = c.get_class_placement_info(foo);

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}