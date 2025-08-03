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


    using state_map = std::map< vmir2::local_index, slot_state >;
    using slot_vec = std::vector< vm_slot >;
    using state_diff = std::map< vmir2::local_index, std::pair< slot_state, slot_state > >;

    class codegen_state_engine
    {
      private:
        std::map< vmir2::local_index, vmir2::slot_state >& state;
        std::vector< vmir2::local_type > const& slot_info;
        vmir2::routine_parameters const& routine_params;

      public:
        codegen_state_engine(std::map< vmir2::local_index, vmir2::slot_state >& state, std::vector< vmir2::local_type > const& slot_info, vmir2::routine_parameters const& params) : state(state), slot_info(slot_info), routine_params(params)
        {
        }

        void apply(vmir2::vm_instruction const& inst)
        {
            rpnx::apply_visitor< void >(
                [&](auto const& x)
                {
                    check_state_valid();
                    apply_internal(x);
                    check_state_valid();
                },
                inst);
        }

        void apply_entry()
        {
            check_state_valid();
            for (local_index i = local_index(0); i < routine_params.positional.size(); i++)
            {
                auto const& param = routine_params.positional[i];
                local_index param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    state[param_slot_index].alive = false;
                    state[param_slot_index].storage_valid = true;
                }
                else
                {
                    state[param_slot_index].alive = true;
                    state[param_slot_index].storage_valid = true;
                }
            }

            for (auto const& [name, param] : routine_params.named)
            {
                auto param_slot_index = param.local_index;

                if (param.type.template type_is< nvalue_slot >())
                {
                    state[param_slot_index].alive = false;
                    state[param_slot_index].storage_valid = true;
                }
                else
                {
                    state[param_slot_index].alive = true;
                    state[param_slot_index].storage_valid = true;
                }
            }
            check_state_valid();
        }

        void check_state_valid()
        {
            for (auto const& [index, st] : state)
            {
                assert(st.valid());
            }
        }

      private:
        void apply_internal(vmir2::assert_instr const& asrt)
        {
            consume(asrt.condition);
        }
        void apply_internal(vmir2::increment const& inc)
        {
            consume(inc.value);
            output(inc.result);
        }
        void apply_internal(vmir2::decrement const& dec)
        {
            consume(dec.value);
            output(dec.result);
        }
        void apply_internal(vmir2::preincrement const& pinc)
        {
            consume(pinc.target);
            output(pinc.target2);
        }
        void apply_internal(vmir2::predecrement const& pdec)
        {
            consume(pdec.target);
            output(pdec.target2);
        }
        void apply_internal(vmir2::to_bool const& tb)
        {
            consume(tb.from);
            output(tb.to);
        }
        void apply_internal(vmir2::to_bool_not const& tbn)
        {
            consume(tbn.from);
            output(tbn.to);
        }
        void apply_internal(vmir2::load_const_zero const& lcz)
        {
            output(lcz.target);
        }

        void apply_internal(vmir2::load_const_bool const& lcb)
        {
            output(lcb.target);
        }
        void apply_internal(vmir2::access_field const& acf)
        {
            consume(acf.base_index);
            output(acf.store_index);
        }
        void apply_internal(vmir2::access_array const& aca)
        {
            consume(aca.base_index);
            consume(aca.index_index);
            output(aca.store_index);
        }
        void apply_internal(vmir2::invoke const& inv)
        {
            check_state_valid();
            auto ivk_func_inst = inv.what.get_as< instanciation_reference >();

            for (std::size_t index = 0; index < inv.args.positional.size(); index++)
            {
                auto const arg_idx = inv.args.positional[index];
                auto const& arg_inst_type = ivk_func_inst.params.positional.at(index);
                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(arg_idx);
                }
                else
                {
                    consume(arg_idx);
                }
            }

            for (auto const& [name, index] : inv.args.named)
            {
                type_symbol arg_inst_type;
                if (name == "RETURN")
                {
                    arg_inst_type = nvalue_slot{.target = slot_info.at(index).type};
                }
                else
                {
                    arg_inst_type = ivk_func_inst.params.named.at(name);
                }

                if (arg_inst_type.template type_is< nvalue_slot >())
                {
                    output(index);
                }
                else
                {
                    consume(index);
                }
            }
            check_state_valid();
        }
        void apply_internal(vmir2::make_reference const& mrf)
        {
            readonly(mrf.value_index);
            output(mrf.reference_index);
        }
        void apply_internal(vmir2::cast_reference const& crf)
        {
            consume(crf.source_ref_index);
            output(crf.target_ref_index);
        }
        void apply_internal(vmir2::constexpr_set_result const& csr)
        {
            consume(csr.target);
        }
        void apply_internal(vmir2::load_const_value const& lcv)
        {
            output(lcv.target);
        }
        void apply_internal(vmir2::make_pointer_to const& mpt)
        {
            readonly(mpt.of_index);
            output(mpt.pointer_index);
        }
        void apply_internal(vmir2::dereference_pointer const& drp)
        {
            consume(drp.from_pointer);
            output(drp.to_reference);
        }
        void apply_internal(vmir2::load_from_ref const& lfr)
        {
            consume(lfr.from_reference);
            output(lfr.to_value);
        }
        void apply_internal(vmir2::ret const& ret)
        {
        }
        void apply_internal(vmir2::int_add const& add)
        {
            consume(add.a);
            consume(add.b);
            output(add.result);
        }
        void apply_internal(vmir2::int_sub const& sub)
        {
            consume(sub.a);
            consume(sub.b);
            output(sub.result);
        }
        void apply_internal(vmir2::int_mul const& mul)
        {
            consume(mul.a);
            consume(mul.b);
            output(mul.result);
        }
        void apply_internal(vmir2::int_div const& div)
        {
            consume(div.a);
            consume(div.b);
            output(div.result);
        }
        void apply_internal(vmir2::int_mod const& mod)
        {
            consume(mod.a);
            consume(mod.b);
            output(mod.result);
        }
        void apply_internal(vmir2::store_to_ref const& str)
        {
            consume(str.from_value);
            consume(str.to_reference);
        }
        void apply_internal(vmir2::load_const_int const& lci)
        {
            output(lci.target);
        }
        void apply_internal(vmir2::cmp_eq const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::cmp_ge const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::cmp_lt const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::cmp_ne const& cmp)
        {
            consume(cmp.a);
            consume(cmp.b);
            output(cmp.result);
        }
        void apply_internal(vmir2::defer_nontrivial_dtor const& dntd)
        {
            readonly(dntd.on_value);
        }
        void apply_internal(vmir2::struct_delegate_new const& dlg)
        {
            state[dlg.on_value].delegates = dlg.fields;
            for (auto const& [name, index] : dlg.fields.named)
            {
                state[index].delegate_of = dlg.on_value;
                state[index].storage_valid = true;
                state[index].dtor_enabled = false;
                state[index].alive = false;
            }

            state[dlg.on_value].storage_valid = true;
            state[dlg.on_value].dtor_enabled = false;
            state[dlg.on_value].alive = true;
        }
        void apply_internal(vmir2::copy_reference const& cpr)
        {
            readonly(cpr.from_index);
            output(cpr.to_index);
        }
        void apply_internal(vmir2::end_lifetime const& elt)
        {
            consume(elt.of);
        }
        void apply_internal(vmir2::pointer_arith const& par)
        {
            consume(par.from);
            consume(par.offset);
            output(par.result);
        }
        void apply_internal(vmir2::pointer_diff const& pdf)
        {
            consume(pdf.from);
            consume(pdf.to);
            output(pdf.result);
        }

        void readonly(local_index idx)
        {
            if (!state[idx].alive || !state[idx].storage_valid)
            {
                throw invalid_instruction_transition_error("readonly input not alive state");
            }
        }
        void consume(local_index idx)
        {
            if (!state[idx].alive || !state[idx].storage_valid)
            {
                throw invalid_instruction_transition_error("consume input not alive state");
            }
            state[idx].alive = false;
            state[idx].storage_valid = state[idx].delegate_of.has_value();
        }
        void output(local_index idx)
        {
            if (state[idx].alive)
            {
                throw invalid_instruction_transition_error("output already set");
            }
            state[idx].alive = true;
            if (!state[idx].storage_valid && state[idx].delegate_of.has_value())
            {
                throw invalid_instruction_transition_error("output not valid");
            }
            state[idx].storage_valid = true;
        }
    };

} // namespace quxlang::vmir2

#endif // RPNX_QUXLANG_STATE_ENGINE_HEADER
