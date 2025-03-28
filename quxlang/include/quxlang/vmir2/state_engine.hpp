//
// Created by Ryan Nicholl on 2024-12-21.
//

#ifndef RPNX_QUXLANG_STATE_ENGINE_HEADER
#define RPNX_QUXLANG_STATE_ENGINE_HEADER

#include <map>
#include <quxlang/vmir2/vmir2.hpp>
#include <vector>

namespace quxlang::vmir2
{
    class state_engine
    {
      public:
        static void apply(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, vm_instruction const& inst);
        static void apply_entry( std::map< vmir2:: storage_index, slot_state > & state,  std::vector< vm_slot > const& slot_info);

        using state_map = std::map< vmir2::storage_index, slot_state >;
        using slot_vec = std::vector< vm_slot >;

      private:
        // Method declarations for each type of instruction
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, to_bool const& tb);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, to_bool_not const& acf);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_zero const& lcz);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, access_field const& acf);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, access_array const& acf);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, invoke const& inv);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, make_reference const& mrf);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, cast_reference const& cst);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, constexpr_set_result const& csr);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_value const& lcv);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, make_pointer_to const& mpt);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, dereference_pointer const& drp);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, load_from_ref const& lfr);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, ret const& ret);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, int_add const& add);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, int_sub const& sub);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, int_mul const& mul);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, int_div const& div);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, int_mod const& mod);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, store_to_ref const& str);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_int const& lci);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_eq const& ceq);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_ne const& cne);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_lt const& clt);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_ge const& cge);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, defer_nontrivial_dtor const& dntd);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, struct_delegate_new const& dlg);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, copy_reference const & cpr);
        static void apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, end_lifetime const & elt);
    };

} // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_STATE_ENGINE_HEADER
