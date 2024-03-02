#include "rylang/backends/llvm/llvm_code_generator.hpp"
#include "rylang/compiler.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/manipulators/mangler.hpp"
#include <iostream>

#include "rylang/data/vm_executable_unit.hpp"

#include "rylang/manipulators/vmmanip.hpp"

#include <rylang/parsers/parse_type_symbol.hpp>

int main(int argc, char** argv)
{
    rylang::output_info target_machine{
        .cpu = rylang::cpu::arm_64,
        .os = rylang::os::macos,
        .binary = rylang::binary::elf,
    };

    rylang::compiler c(argc, argv, target_machine);

    rylang::llvm_code_generator cg(target_machine);
    //cg.foo();
    //return 0;

    rylang::type_symbol cn = rylang::module_reference{"main"};
    cn = rylang::subentity_reference{cn, "quz"};
    cn = rylang::subentity_reference{std::move(cn), "bif"};
    cn = rylang::subentity_reference{std::move(cn), "box"};
    cn = rylang::subentity_reference{std::move(cn), "buz"};

    rylang::primitive_type_integer_reference u32type = rylang::primitive_type_integer_reference{32, true};

    rylang::call_parameter_information args;
    args.argument_types.push_back(u32type);
    args.argument_types.push_back(u32type);

    auto qn = c.get_function_qualname(cn, args);

    std::string name = rylang::mangle(qn);

    std::set< rylang::type_symbol > already_compiled;
    std::set< rylang::type_symbol > already_assembled;

    std::set< rylang::type_symbol > new_deps_to_compile = {qn};
    std::set< rylang::type_symbol > new_deps_to_assemble;

    std::map< rylang::type_symbol, std::vector< std::byte > > compiled_code;

    while (new_deps_to_compile.empty() == false || new_deps_to_assemble.empty() == false)
    {
        if (!new_deps_to_compile.empty())
        {
            auto to_compile = *new_deps_to_compile.begin();

            std::cout << "Compiling " << rylang::to_string(to_compile) << std::endl;
            rylang::vm_procedure vmf = c.get_vm_procedure_from_canonical_functanoid(to_compile);
            // std::cout << rylang::to_string(rylang::vm_executable_unit{vmf.body}) << std::endl;
            auto code = cg.get_function_code(rylang::cpu_arch_armv8a(), vmf);
            compiled_code[to_compile] = code;
            already_compiled.insert(to_compile);

            for (auto& x : vmf.invoked_functanoids)
            {
                if (already_compiled.contains(x))
                    continue;
                else
                    new_deps_to_compile.insert(x);
            }
            new_deps_to_compile.erase(to_compile);

            for (auto & x: vmf.invoked_asm_procedures)
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

            rylang::asm_procedure proc = c.get_asm_procedure_from_canonical_symbol(to_assemble);
        }
    }

    // std::cout << "Got overload:" << name << std::endl;
    //  auto vec = cg.get_function_code(rylang::cpu_arch_armv8a(), func_name );
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

    // return 0;
    rylang::type_symbol foo = rylang::instanciation_reference{rylang::subentity_reference{rylang::module_reference{"main"}, "box2"}, {rylang::parsers::parse_type_symbol("I32")}};

    auto foo_placement_info = c.get_class_placement_info(foo);

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}