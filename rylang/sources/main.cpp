#include "rylang/compiler.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/llvm_code_generator.hpp"
#include "rylang/manipulators/mangler.hpp"
#include <iostream>

#include "rylang/data/vm_executable_unit.hpp"

#include "rylang/manipulators/vmmanip.hpp"


int main(int argc, char** argv)
{
    rylang::compiler c(argc, argv);

    rylang::llvm_code_generator cg(&c);

    rylang::qualified_symbol_reference cn = rylang::module_reference{"main"};
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

    rylang::vm_procedure vmf = c.get_vm_procedure_from_canonical_functanoid(qn);

    std::cout << rylang::to_string(rylang::vm_executable_unit{vmf.body}) << std::endl;
    auto code = cg.get_function_code(rylang::cpu_arch_armv8a(), vmf);


    std::cout << "Got overload:" << name << std::endl;
    // auto vec = cg.get_function_code(rylang::cpu_arch_armv8a(), func_name );
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

    rylang::qualified_symbol_reference foo = rylang::subentity_reference{rylang::module_reference{"main"}, "foo"};

    auto foo_placement_info = c.get_class_placement_info(foo);

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}