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
        void apply(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, vm_instruction const& inst);

      private:
        // Method declarations for each type of instruction
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_zero const& lcz);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, access_field const& acf);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, invoke const& inv);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, make_reference const& mrf);

        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, cast_reference const& cst);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, constexpr_set_result const& csr);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_value const& lcv);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, make_pointer_to const& mpt);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, dereference_pointer const& drp);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, load_from_ref const& lfr);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, ret const& ret);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, int_add const& add);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, int_sub const& sub);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, int_mul const& mul);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, int_div const& div);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, int_mod const& mod);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, store_to_ref const& str);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, load_const_int const& lci);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_eq const& ceq);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_ne const& cne);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_lt const& clt);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, cmp_ge const& cge);
        void apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, defer_nontrivial_dtor const& dntd);
    };

} // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_STATE_ENGINE_HEADER
