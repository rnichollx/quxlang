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
    private:
        std::map< vmir2::storage_index, slot_state >& state;
        std::vector< vm_slot > const& slot_info;
    public:
        state_engine(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info)
            : state(state), slot_info(slot_info)
        {
        }

        void apply(vm_instruction const& inst);
        void apply_entry();

        void check_state_valid();

        using state_map = std::map< vmir2::storage_index, slot_state >;
        using slot_vec = std::vector< vm_slot >;
        using state_diff = std::map< vmir2::storage_index, std::pair< slot_state, slot_state > >;

    private:
        void apply_internal(assert_instr const &asrt);
        void apply_internal(increment const& tb);
        void apply_internal(decrement const& acf);
        void apply_internal(preincrement const& tb);
        void apply_internal(predecrement const& acf);
        void apply_internal(to_bool const& tb);
        void apply_internal(to_bool_not const& acf);
        void apply_internal(load_const_zero const& lcz);
        void apply_internal(access_field const& acf);
        void apply_internal(access_array const& acf);
        void apply_internal(invoke const& inv);
        void apply_internal(make_reference const& mrf);
        void apply_internal(cast_reference const& cst);
        void apply_internal(constexpr_set_result const& csr);
        void apply_internal(load_const_value const& lcv);
        void apply_internal(make_pointer_to const& mpt);
        void apply_internal(dereference_pointer const& drp);
        void apply_internal(load_from_ref const& lfr);
        void apply_internal(ret const& ret);
        void apply_internal(int_add const& add);
        void apply_internal(int_sub const& sub);
        void apply_internal(int_mul const& mul);
        void apply_internal(int_div const& div);
        void apply_internal(int_mod const& mod);
        void apply_internal(store_to_ref const& str);
        void apply_internal(load_const_int const& lci);
        void apply_internal(cmp_eq const& ceq);
        void apply_internal(cmp_ne const& cne);
        void apply_internal(cmp_lt const& clt);
        void apply_internal(cmp_ge const& cge);
        void apply_internal(defer_nontrivial_dtor const& dntd);
        void apply_internal(struct_delegate_new const& dlg);
        void apply_internal(copy_reference const& cpr);
        void apply_internal(end_lifetime const& elt);
        void apply_internal(pointer_arith const& par);
        void apply_internal(pointer_diff const& par);

        void mustbe_inttype(storage_index idx);
        void mustbe_booltype(storage_index idx);
        void mustbe_pointertype(storage_index idx);
        void mustbe_reftype(storage_index idx);

        void readonly(storage_index idx);
        void consume(storage_index idx);
        void output(storage_index idx);
    };

    class state_engine2
    {
    private:
        std::map< vmir2::storage_index, slot_state >& state;
        std::vector< slottype > const& slot_info;
    public:
        state_engine2(std::map< vmir2::storage_index, slot_state >& state, std::vector< slottype > const& slot_info)
            : state(state), slot_info(slot_info)
        {
        }

        void apply(vm_instruction const& inst);
        void apply_entry();

        void check_state_valid();

        using state_map = std::map< vmir2::storage_index, slot_state >;
        using slot_vec = std::vector< vm_slot >;
        using state_diff = std::map< vmir2::storage_index, std::pair< slot_state, slot_state > >;

    private:
        void apply_internal(assert_instr const &asrt);
        void apply_internal(increment const& tb);
        void apply_internal(decrement const& acf);
        void apply_internal(preincrement const& tb);
        void apply_internal(predecrement const& acf);
        void apply_internal(to_bool const& tb);
        void apply_internal(to_bool_not const& acf);
        void apply_internal(load_const_zero const& lcz);
        void apply_internal(access_field const& acf);
        void apply_internal(access_array const& acf);
        void apply_internal(invoke const& inv);
        void apply_internal(make_reference const& mrf);
        void apply_internal(cast_reference const& cst);
        void apply_internal(constexpr_set_result const& csr);
        void apply_internal(load_const_value const& lcv);
        void apply_internal(make_pointer_to const& mpt);
        void apply_internal(dereference_pointer const& drp);
        void apply_internal(load_from_ref const& lfr);
        void apply_internal(ret const& ret);
        void apply_internal(int_add const& add);
        void apply_internal(int_sub const& sub);
        void apply_internal(int_mul const& mul);
        void apply_internal(int_div const& div);
        void apply_internal(int_mod const& mod);
        void apply_internal(store_to_ref const& str);
        void apply_internal(load_const_int const& lci);
        void apply_internal(cmp_eq const& ceq);
        void apply_internal(cmp_ne const& cne);
        void apply_internal(cmp_lt const& clt);
        void apply_internal(cmp_ge const& cge);
        void apply_internal(defer_nontrivial_dtor const& dntd);
        void apply_internal(struct_delegate_new const& dlg);
        void apply_internal(copy_reference const& cpr);
        void apply_internal(end_lifetime const& elt);
        void apply_internal(pointer_arith const& par);
        void apply_internal(pointer_diff const& par);

        void mustbe_inttype(storage_index idx);
        void mustbe_booltype(storage_index idx);
        void mustbe_pointertype(storage_index idx);
        void mustbe_reftype(storage_index idx);

        void readonly(storage_index idx);
        void consume(storage_index idx);
        void output(storage_index idx);
    };

} // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_STATE_ENGINE_HEADER
