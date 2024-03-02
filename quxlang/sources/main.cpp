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
        .cpu = quxlang::cpu::arm_64,
        .os = quxlang::os::macos,
        .binary = quxlang::binary::elf,
    };

    quxlang::compiler c(argc, argv, target_machine);

    quxlang::llvm_code_generator cg(target_machine);
    //cg.foo();
    //return 0;

    quxlang::type_symbol cn = quxlang::module_reference{"main"};
    cn = quxlang::subentity_reference{cn, "quz"};
    cn = quxlang::subentity_reference{std::move(cn), "bif"};
    cn = quxlang::subentity_reference{std::move(cn), "box"};
    cn = quxlang::subentity_reference{std::move(cn), "buz"};

    quxlang::primitive_type_integer_reference u32type = quxlang::primitive_type_integer_reference{32, true};

    quxlang::call_parameter_information args;
    args.argument_types.push_back(u32type);
    args.argument_types.push_back(u32type);

    auto qn = c.get_function_qualname(cn, args);

    std::string name = quxlang::mangle(qn);


    std::string boxy_input = "::boxy";
    std::string::iterator boxy_iter = boxy_input.begin();
    auto boxy = quxlang::parsers::parse_type_symbol<std::string::iterator>(boxy_iter, boxy_input.end());

    std::set< quxlang::type_symbol > already_compiled;
    std::set< quxlang::type_symbol > already_assembled;

    std::set< quxlang::type_symbol > new_deps_to_compile = {qn};
    std::set< quxlang::type_symbol > new_deps_to_assemble = { boxy };

    std::map< quxlang::type_symbol, std::vector< std::byte > > compiled_code;

    while (new_deps_to_compile.empty() == false || new_deps_to_assemble.empty() == false)
    {
        if (!new_deps_to_compile.empty())
        {
            auto to_compile = *new_deps_to_compile.begin();

            std::cout << "Compiling " << quxlang::to_string(to_compile) << std::endl;
            quxlang::vm_procedure vmf = c.get_vm_procedure_from_canonical_functanoid(to_compile);
            // std::cout << quxlang::to_string(quxlang::vm_executable_unit{vmf.body}) << std::endl;
            auto code = cg.get_function_code(quxlang::cpu_arch_armv8a(), vmf);
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

            quxlang::asm_procedure proc = c.get_asm_procedure_from_canonical_symbol(to_assemble);

            auto code = cg.assemble(proc, target_machine.cpu);
            compiled_code[to_assemble] = code;

            new_deps_to_assemble.erase(to_assemble);
        }
    }

    // std::cout << "Got overload:" << name << std::endl;
    //  auto vec = cg.get_function_code(quxlang::cpu_arch_armv8a(), func_name );
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
    quxlang::type_symbol foo = quxlang::instanciation_reference{quxlang::subentity_reference{quxlang::module_reference{"main"}, "box2"}, {quxlang::parsers::parse_type_symbol("I32")}};

    auto foo_placement_info = c.get_class_placement_info(foo);

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}