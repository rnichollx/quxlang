#ifndef QUXLANG_VMIR2_ASSEMBLY_HEADER_GUARD
#define QUXLANG_VMIR2_ASSEMBLY_HEADER_GUARD

#include "state_engine.hpp"

#include <quxlang/vmir2/vmir2.hpp>
#include <string>

namespace quxlang::vmir2
{
    class assembler
    {
      public:
        std::string to_string(vmir2::functanoid_routine3 inst);
        std::string to_string(vmir2::vm_instruction inst);
        std::string to_string(vmir2::vm_terminator inst);
        std::string to_string(vmir2::local_type inst);
        std::string to_string(vmir2::vm_slot slt);
        std::string to_string(vmir2::executable_block const &block);
        std::string to_string(vmir2::state_map const & state);


        assembler(vmir2::functanoid_routine3 what) : m_what(what) {}
      private:
        vmir2::functanoid_routine3 m_what;
        vmir2::state_map state;

        void set_arg_state();

        std::string to_string_internal(vmir2::assert_instr const &asrt);
        std::string to_string_internal(vmir2::increment inst);
        std::string to_string_internal(vmir2::decrement inst);
        std::string to_string_internal(vmir2::preincrement inst);
        std::string to_string_internal(vmir2::predecrement inst);
        std::string to_string_internal(vmir2::to_bool inst);
        std::string to_string_internal(vmir2::to_bool_not inst);

        std::string to_string_internal(vmir2::access_field inst);
        std::string to_string_internal(vmir2::access_array inst);
        std::string to_string_internal(vmir2::invoke inst);
        std::string to_string_internal(vmir2::invocation_args inst);
        std::string to_string_internal(vmir2::make_reference inst);
        std::string to_string_internal(vmir2::cast_reference inst);
        std::string to_string_internal(vmir2::copy_reference cpr);
        std::string to_string_internal(vmir2::constexpr_set_result inst);
        std::string to_string_internal(vmir2::load_const_value inst);
        std::string to_string_internal(vmir2::load_const_bool inst);
        std::string to_string_internal(vmir2::load_const_zero inst);
        std::string to_string_internal(vmir2::load_const_int inst);
        std::string to_string_internal(vmir2::make_pointer_to inst);
        std::string to_string_internal(vmir2::load_from_ref inst);
        std::string to_string_internal(vmir2::store_to_ref inst);
        std::string to_string_internal(vmir2::dereference_pointer inst);
        std::string to_string_internal(vmir2::defer_nontrivial_dtor dntd);
        std::string to_string_internal(vmir2::struct_delegate_new sdn);
        std::string to_string_internal(vmir2::struct_complete_new scn);
        std::string to_string_internal(vmir2::end_lifetime elt);

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
        std::string to_string_internal(vmir2::pointer_arith inst);
        std::string to_string_internal(vmir2::pointer_diff inst);

    };

}; // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_ASSEMBLY_HEADER

