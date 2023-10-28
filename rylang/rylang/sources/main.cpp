#include "rylang/compiler.hpp"
#include "rylang/llvm_code_generator.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    rylang::compiler c(argc, argv);

    rylang::llvm_code_generator cg(&c);

    auto func_name = rylang::canonical_resolved_function_chain{};
    func_name.function_entity_chain = rylang::canonical_lookup_chain{"main"};
    func_name.overload_index = 0;
    auto vec = cg.get_function_code(rylang::cpu_arch_armv8a(), func_name );
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

    auto foo_placement_info = c.get_class_placement_info(rylang::canonical_lookup_chain{"foo"});

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}