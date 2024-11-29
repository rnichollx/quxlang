// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BUILTIN_CLASSIFIER_HEADER_GUARD
#define QUXLANG_BUILTIN_CLASSIFIER_HEADER_GUARD
#include "data/machine.hpp"
#include "data/type_symbol.hpp"
#include "vmir2/vmir2.hpp"

namespace quxlang
{

    class intrinsic_builtin_classifier
    {
         output_info machine_info_;
         vmir2::executable_block_generation_state const & state_;
    public:
        intrinsic_builtin_classifier(output_info const & m, vmir2::executable_block_generation_state const & state) : machine_info_(m), state_(state)  {}

        std::vector<signature> list_intrinsics(type_symbol func);

        std::vector< signature > list_constructor(type_symbol of_type);
        std::map< std::string, quxlang::signature > list_builtin_binary_operator(type_symbol of_type, std::string_view oper, bool rhs);
        std::vector<signature> list_builtin_prefix_operator(type_symbol of_type);

        std::optional<vmir2::vm_instruction> intrinsic_instruction(type_symbol func, vmir2::invocation_args args);


        bool is_intrinsic_type(type_symbol of_type);

    };
}


#endif //BUILTIN_CLASSIFIER_H
