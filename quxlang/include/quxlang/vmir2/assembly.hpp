// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_ASSEMBLY_HEADER_GUARD
#define QUXLANG_VMIR2_ASSEMBLY_HEADER_GUARD

#include <quxlang/vmir2/vmir2.hpp>
#include <string>

namespace quxlang::vmir2
{
    class assembler
    {
      public:
        std::string to_string(vmir2::functanoid_routine2 inst);
        std::string to_string(vmir2::vm_instruction inst);
        std::string to_string(vmir2::vm_terminator inst);
        std::string to_string(vmir2::vm_slot slt);
        std::string to_string(vmir2::executable_block const &block);


        assembler(vmir2::functanoid_routine2 what) : m_what(what) {}
      private:
        vmir2::functanoid_routine2 m_what;

        std::string to_string_internal(vmir2::access_field inst);
        std::string to_string_internal(vmir2::invoke inst);
        std::string to_string_internal(vmir2::invocation_args inst);
        std::string to_string_internal(vmir2::make_reference inst);
        std::string to_string_internal(vmir2::cast_reference inst);
        std::string to_string_internal(vmir2::constexpr_set_result inst);
        std::string to_string_internal(vmir2::load_const_value inst);
        std::string to_string_internal(vmir2::load_const_zero inst);
        std::string to_string_internal(vmir2::load_const_int inst);
        std::string to_string_internal(vmir2::make_pointer_to inst);
        std::string to_string_internal(vmir2::load_from_ref inst);
        std::string to_string_internal(vmir2::move_value inst);
        std::string to_string_internal(vmir2::store_to_ref inst);
        std::string to_string_internal(vmir2::dereference_pointer inst);

        std::string to_string_internal(vmir2::int_add add);
        std::string to_string_internal(vmir2::int_sub sub);
        std::string to_string_internal(vmir2::int_mul mul);
        std::string to_string_internal(vmir2::int_div div);
        std::string to_string_internal(vmir2::int_mod mod);

        std::string to_string_internal(vmir2::cmp_eq inst);
        std::string to_string_internal(vmir2::cmp_ne inst);
        std::string to_string_internal(vmir2::cmp_lt inst);
        std::string to_string_internal(vmir2::cmp_ge inst);


        std::string to_string_internal(vmir2::jump inst);
        std::string to_string_internal(vmir2::branch inst);
        std::string to_string_internal(vmir2::ret inst);


    };

}; // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_ASSEMBLY_HEADER
