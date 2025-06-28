//
// Created by Ryan Nicholl on 2024-12-21.
//
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/exception.hpp"

// Never use .at( ) when checking state as this code should never throw out_of_range.
// Use operator[] instead.
// It may not already exist, which should be treated as a dead slot.
// We want to throw invalid_instruction_transition_error if we try to access a dead slot,
// instead of out_of_range.

// It's okay for the code to throw out_of_range if we try to access a slot_info that doesn't exist, but never for slot state.

void quxlang::vmir2::state_engine::apply(quxlang::vmir2::vm_instruction const& inst)
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

void quxlang::vmir2::state_engine2::apply(quxlang::vmir2::vm_instruction const& inst)
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

void quxlang::vmir2::state_engine::apply_entry()
{
    check_state_valid();
    for (local_index i = local_index(0); i < slot_info.size(); i++)
    {
        auto const& slot = slot_info[i];

        if (slot.kind == slot_kind::named_arg || slot.kind == slot_kind::positional_arg)
        {
            if (!typeis< nvalue_slot >(slot.type))
            {
                state[i].alive = true;
                state[i].storage_valid = true;
            }
            else
            {
                state[i].alive = false;
                state[i].storage_valid = true;
            }
        }
    }
    check_state_valid();
}

void quxlang::vmir2::state_engine2::apply_entry()
{
    check_state_valid();

    for (local_index i = local_index(0); i < this->routine_params.positional.size(); i++)
    {
        auto const& param = this->routine_params.positional[i];

        local_index param_slot_index = param.assign_index;


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

    for (auto const& [name, param] : this->routine_params.named)
    {
        auto param_slot_index = param.assign_index;


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

void quxlang::vmir2::state_engine::check_state_valid()
{
    for (auto const& [index, st] : state)
    {
        int x = 0;
        auto xy = st;
        assert(st.valid());
    }
}

void quxlang::vmir2::state_engine2::check_state_valid()
{
    for (auto const& [index, st] : state)
    {
        int x = 0;
        auto xy = st;
        assert(st.valid());
    }
}

void quxlang::vmir2::state_engine::apply_internal(assert_instr const& asrt)
{
    consume(asrt.condition);
}

void quxlang::vmir2::state_engine2::apply_internal(assert_instr const& asrt)
{
    consume(asrt.condition);
}

void quxlang::vmir2::state_engine::apply_internal(increment const& inc)
{
    consume(inc.value);
    output(inc.result);
}

void quxlang::vmir2::state_engine2::apply_internal(increment const& inc)
{
    consume(inc.value);
    output(inc.result);
}

void quxlang::vmir2::state_engine::apply_internal(decrement const& dec)
{
    consume(dec.value);
    output(dec.result);
}

void quxlang::vmir2::state_engine2::apply_internal(decrement const& dec)
{
    consume(dec.value);
    output(dec.result);
}

void quxlang::vmir2::state_engine::apply_internal(preincrement const& pinc)
{
    consume(pinc.target);
    output(pinc.target2);
}

void quxlang::vmir2::state_engine2::apply_internal(preincrement const& pinc)
{
    consume(pinc.target);
    output(pinc.target2);
}

void quxlang::vmir2::state_engine::apply_internal(predecrement const& pdec)
{
    consume(pdec.target);
    output(pdec.target2);
}

void quxlang::vmir2::state_engine2::apply_internal(predecrement const& pdec)
{
    consume(pdec.target);
    output(pdec.target2);
}

void quxlang::vmir2::state_engine::apply_internal(to_bool const& tb)
{
    consume(tb.from);
    output(tb.to);
}

void quxlang::vmir2::state_engine2::apply_internal(to_bool const& tb)
{
    consume(tb.from);
    output(tb.to);
}

void quxlang::vmir2::state_engine::apply_internal(to_bool_not const& tbn)
{
    consume(tbn.from);
    output(tbn.to);
}

void quxlang::vmir2::state_engine2::apply_internal(to_bool_not const& tbn)
{
    consume(tbn.from);
    output(tbn.to);
}

void quxlang::vmir2::state_engine::apply_internal(load_const_zero const& lcz)
{
    output(lcz.target);
}

void quxlang::vmir2::state_engine2::apply_internal(load_const_zero const& lcz)
{
    output(lcz.target);
}

void quxlang::vmir2::state_engine::apply_internal(access_field const& acf)
{
    consume(acf.base_index);
    output(acf.store_index);
}

void quxlang::vmir2::state_engine2::apply_internal(access_field const& acf)
{
    consume(acf.base_index);
    output(acf.store_index);
}

void quxlang::vmir2::state_engine::apply_internal(access_array const& aca)
{
    consume(aca.base_index);
    consume(aca.index_index);
    output(aca.store_index);
}

void quxlang::vmir2::state_engine2::apply_internal(access_array const& aca)
{
    consume(aca.base_index);
    consume(aca.index_index);
    output(aca.store_index);
}

void quxlang::vmir2::state_engine::apply_internal(invoke const& inv)
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

void quxlang::vmir2::state_engine2::apply_internal(invoke const& inv)
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

void quxlang::vmir2::state_engine::apply_internal(make_reference const& mrf)
{
    readonly(mrf.value_index);
    output(mrf.reference_index);
}

void quxlang::vmir2::state_engine2::apply_internal(make_reference const& mrf)
{
    readonly(mrf.value_index);
    output(mrf.reference_index);
}

void quxlang::vmir2::state_engine::apply_internal(cast_reference const& crf)
{
    consume(crf.source_ref_index);
    output(crf.target_ref_index);
}

void quxlang::vmir2::state_engine2::apply_internal(cast_reference const& crf)
{
    consume(crf.source_ref_index);
    output(crf.target_ref_index);
}

void quxlang::vmir2::state_engine::apply_internal(constexpr_set_result const& csr)
{
    if (slot_info.at(csr.target).kind != slot_kind::literal)
    {
        // Special exception, CSR can operate on literals for now.
        // TODO: Get rid of this exception because it's messy.
        consume(csr.target);
    }
}

void quxlang::vmir2::state_engine2::apply_internal(constexpr_set_result const& csr)
{
    consume(csr.target);
}

void quxlang::vmir2::state_engine::apply_internal(load_const_value const& lcv)
{
    output(lcv.target);
}

void quxlang::vmir2::state_engine2::apply_internal(load_const_value const& lcv)
{
    output(lcv.target);
}

void quxlang::vmir2::state_engine::apply_internal(make_pointer_to const& mpt)
{
    readonly(mpt.of_index);
    output(mpt.pointer_index);
}

void quxlang::vmir2::state_engine2::apply_internal(make_pointer_to const& mpt)
{
    readonly(mpt.of_index);
    output(mpt.pointer_index);
}

void quxlang::vmir2::state_engine::apply_internal(dereference_pointer const& drp)
{
    consume(drp.from_pointer);
    output(drp.to_reference);
}

void quxlang::vmir2::state_engine2::apply_internal(dereference_pointer const& drp)
{
    consume(drp.from_pointer);
    output(drp.to_reference);
}

void quxlang::vmir2::state_engine::apply_internal(load_from_ref const& lfr)
{
    consume(lfr.from_reference);
    output(lfr.to_value);
}

void quxlang::vmir2::state_engine2::apply_internal(load_from_ref const& lfr)
{
    consume(lfr.from_reference);
    output(lfr.to_value);
}

void quxlang::vmir2::state_engine::apply_internal(ret const& ret)
{
}

void quxlang::vmir2::state_engine2::apply_internal(ret const& ret)
{
}

void quxlang::vmir2::state_engine::apply_internal(int_add const& add)
{
    consume(add.a);
    consume(add.b);
    output(add.result);
}

void quxlang::vmir2::state_engine2::apply_internal(int_add const& add)
{
    consume(add.a);
    consume(add.b);
    output(add.result);
}

void quxlang::vmir2::state_engine::apply_internal(int_sub const& sub)
{
    consume(sub.a);
    consume(sub.b);
    output(sub.result);
}

void quxlang::vmir2::state_engine2::apply_internal(int_sub const& sub)
{
    consume(sub.a);
    consume(sub.b);
    output(sub.result);
}

void quxlang::vmir2::state_engine::apply_internal(int_mul const& mul)
{
    consume(mul.a);
    consume(mul.b);
    output(mul.result);
}

void quxlang::vmir2::state_engine2::apply_internal(int_mul const& mul)
{
    consume(mul.a);
    consume(mul.b);
    output(mul.result);
}

void quxlang::vmir2::state_engine::apply_internal(int_div const& div)
{
    consume(div.a);
    consume(div.b);
    output(div.result);
}

void quxlang::vmir2::state_engine2::apply_internal(int_div const& div)
{
    consume(div.a);
    consume(div.b);
    output(div.result);
}

void quxlang::vmir2::state_engine::apply_internal(int_mod const& mod)
{
    consume(mod.a);
    consume(mod.b);
    output(mod.result);
}

void quxlang::vmir2::state_engine2::apply_internal(int_mod const& mod)
{
    consume(mod.a);
    consume(mod.b);
    output(mod.result);
}

void quxlang::vmir2::state_engine::apply_internal(store_to_ref const& str)
{
    consume(str.from_value);
    consume(str.to_reference);
}

void quxlang::vmir2::state_engine2::apply_internal(store_to_ref const& str)
{
    consume(str.from_value);
    consume(str.to_reference);
}

void quxlang::vmir2::state_engine::apply_internal(load_const_int const& lci)
{
    output(lci.target);
}

void quxlang::vmir2::state_engine2::apply_internal(load_const_int const& lci)
{
    output(lci.target);
}

void quxlang::vmir2::state_engine::apply_internal(cmp_eq const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine2::apply_internal(cmp_eq const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine::apply_internal(cmp_ge const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine2::apply_internal(cmp_ge const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine::apply_internal(cmp_lt const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine2::apply_internal(cmp_lt const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine::apply_internal(cmp_ne const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine2::apply_internal(cmp_ne const& str)
{
    consume(str.a);
    consume(str.b);
    output(str.result);
}

void quxlang::vmir2::state_engine::apply_internal(defer_nontrivial_dtor const& str)
{
    readonly(str.on_value);
}

void quxlang::vmir2::state_engine2::apply_internal(defer_nontrivial_dtor const& str)
{
    readonly(str.on_value);
}

void quxlang::vmir2::state_engine::apply_internal(struct_delegate_new const& dlg)
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

void quxlang::vmir2::state_engine2::apply_internal(struct_delegate_new const& dlg)
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

void quxlang::vmir2::state_engine::apply_internal(copy_reference const& cpr)
{
    readonly(cpr.from_index);
    output(cpr.to_index);
}

void quxlang::vmir2::state_engine2::apply_internal(copy_reference const& cpr)
{
    readonly(cpr.from_index);
    output(cpr.to_index);
}

void quxlang::vmir2::state_engine::apply_internal(end_lifetime const& elt)
{
    consume(elt.of);
}

void quxlang::vmir2::state_engine2::apply_internal(end_lifetime const& elt)
{
    consume(elt.of);
}

void quxlang::vmir2::state_engine::apply_internal(pointer_arith const& par)
{
    consume(par.from);
    consume(par.offset);
    output(par.result);
}

void quxlang::vmir2::state_engine2::apply_internal(pointer_arith const& par)
{
    consume(par.from);
    consume(par.offset);
    output(par.result);
}

void quxlang::vmir2::state_engine::apply_internal(pointer_diff const& pdf)
{
    consume(pdf.from);
    consume(pdf.to);
    output(pdf.result);
}

void quxlang::vmir2::state_engine2::apply_internal(pointer_diff const& pdf)
{
    consume(pdf.from);
    consume(pdf.to);
    output(pdf.result);
}

void quxlang::vmir2::state_engine::readonly(local_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("readonly input not alive state");
    }
}
void quxlang::vmir2::state_engine2::readonly(local_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("readonly input not alive state");
    }
}
void quxlang::vmir2::state_engine::consume(local_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("consume input not alive state");
    }
    state[idx].alive = false;
    state[idx].storage_valid = state[idx].delegate_of.has_value();
}

void quxlang::vmir2::state_engine2::consume(local_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("consume input not alive state");
    }
    state[idx].alive = false;
    state[idx].storage_valid = state[idx].delegate_of.has_value();
}
void quxlang::vmir2::state_engine::output(local_index idx)
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

void quxlang::vmir2::state_engine2::output(local_index idx)
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
