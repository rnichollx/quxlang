#include "rylang/compiler.hpp"
#include "rylang/converters/qual_converters.hpp"
#include "rylang/llvm_code_generator.hpp"
#include "rylang/manipulators/mangler.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    rylang::compiler c(argc, argv);

    rylang::llvm_code_generator cg(&c);

    auto func_name = rylang::canonical_resolved_function_chain{};
    func_name.function_entity_chain = rylang::canonical_lookup_chain{"main"};
    func_name.overload_index = 0;
    rylang::vm_procedure vmf;

    auto u32 = rylang::vm_type_int{32, false};
    vmf.interface.argument_types.push_back(u32);
    vmf.interface.argument_types.push_back(u32);
    vmf.interface.return_type = u32;

    vmf.body.code.push_back(rylang::vm_allocate_storage{4, 4});
    rylang::vm_store save_value;
    save_value.what = rylang::vm_expr_primitive_binary_op{rylang::vm_expr_dereference{rylang::vm_expr_load_address{0}, u32}, rylang::vm_expr_dereference{rylang::vm_expr_load_address{1}, u32},
                                                          rylang::vm_primitive_binary_operator::add};
    save_value.where = rylang::vm_expr_load_address{2};
    save_value.type = u32;
    vmf.body.code.push_back(save_value);
    vmf.body.code.push_back(rylang::vm_return{rylang::vm_expr_dereference{rylang::vm_expr_load_address{2}, u32}});

    auto code = cg.get_function_code(rylang::cpu_arch_armv8a(), vmf);

    rylang::canonical_lookup_chain cn{"quz", "bif", "box", "buz"};
    rylang::canonical_type_reference u32type = rylang::integral_keyword_ast{true, 32};

    rylang::call_overload_set args;
    args.argument_types.push_back(u32type);
    args.argument_types.push_back(u32type);

    rylang::qualified_symbol_reference ql = rylang::convert_to_qualified_symbol_reference(cn);
    auto qn = c.get_function_qualname(ql, args);

    std::string name = rylang::mangle(qn);

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

    auto foo_placement_info = c.get_class_placement_info(rylang::canonical_lookup_chain{"foo"});

    std::cout << "Foo size: " << foo_placement_info.size << "\n";

    std::cout << "Foo alignment: " << foo_placement_info.alignment << "\n";
}