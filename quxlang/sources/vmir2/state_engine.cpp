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

void quxlang::vmir2::state_engine::apply_entry()
{
    check_state_valid();
    for (std::size_t i = 0; i < slot_info.size(); i++)
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
            }
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

void quxlang::vmir2::state_engine::apply_internal(increment const& inc)
{
    consume(inc.value);
    output(inc.result);
}

void quxlang::vmir2::state_engine::apply_internal(decrement const& dec)
{
    consume(dec.target);
    output(dec.oldval);
}

void quxlang::vmir2::state_engine::apply_internal(preincrement const& pinc)
{
    if (!state.at(pinc.target).alive)
    {
        throw invalid_instruction_transition_error("Attempt to increment a dead slot");
    }

    if (state[pinc.target2].alive)
    {
        throw invalid_instruction_transition_error("Attempt to increment a non-dead slot");
    }

    state[pinc.target].alive = false;
    state[pinc.target2].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(predecrement const& pdec)
{
    if (!state[pdec.target].alive)
    {
        throw invalid_instruction_transition_error("Attempt to increment a dead slot");
    }

    if (state[pdec.target2].alive)
    {
        throw invalid_instruction_transition_error("Attempt to increment a non-dead slot");
    }

    state[pdec.target].alive = false;
    state[pdec.target2].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(to_bool const& tb)
{
    if (!state[tb.from].alive)
    {
        throw invalid_instruction_transition_error("from entry must be live");
    }
    if (state[tb.to].alive)
    {
        throw invalid_instruction_transition_error("to entry must be dead");
    }

    state[tb.from].alive = false;
    state[tb.to].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(to_bool_not const& tbn)
{
    if (!state[tbn.from].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a dead slot");
    }
    if (state[tbn.to].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[tbn.from].alive = false;
    state[tbn.to].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(load_const_zero const& lcz)
{
    output(lcz.target);
}

void quxlang::vmir2::state_engine::apply_internal(access_field const& acf)
{
    if (!state.at(acf.base_index).alive)
    {
        throw invalid_instruction_transition_error("Attempt to access field of a dead slot");
    }

    if (state[acf.store_index].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[acf.store_index].alive = true;
    state[acf.base_index].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(access_array const& aca)
{
    // TODO: add state checks

    state[aca.base_index].alive = false;
    state[aca.index_index].alive = false;
    state[aca.store_index].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(invoke const& inv)
{
    check_state_valid();
    auto ivk_func_inst = inv.what.get_as< instanciation_reference >();

    for (std::size_t index = 0; index < inv.args.positional.size(); index++)
    {
        auto arg_inst_type = ivk_func_inst.params.positional.at(index);

        if (arg_inst_type.template type_is< nvalue_slot >())
        {
            output(inv.args.positional[index]);
        }
        else
        {
            consume(inv.args.positional[index]);
        }
    }

    for (auto const& [name, index] : inv.args.named)
    {
        bool arg_alive = state[index].alive;

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
    if (state.at(mrf.value_index).storage_valid == false)
    {
        // throw invalid_instruction_transition_error("Attempt to make reference to a dead slot");
    }

    if (state[mrf.reference_index].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[mrf.reference_index].alive = true;

    // MRF makes a reference without destroying the original value
}

void quxlang::vmir2::state_engine::apply_internal(cast_reference const& crf)
{
    if (state[crf.source_ref_index].alive == false)
    {
        throw invalid_instruction_transition_error("Attempt to cast reference to a dead slot");
    }
    if (state[crf.target_ref_index].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[crf.target_ref_index].alive = true;
    state[crf.source_ref_index].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(constexpr_set_result const& csr)
{
    // Special case: CSR with numeric literal should just return that numeric literal

    if (slot_info.at(csr.target).kind == slot_kind::literal)
    {
        if (slot_info.at(csr.target).type.template type_is< numeric_literal_reference >())
        {
            return;
        }
    }

    if (!state.at(csr.target).alive)
    {
        throw invalid_instruction_transition_error("input slot is dead");
    }

    state[csr.target].dtor_enabled = false;
    state[csr.target].storage_valid = false;

    state[csr.target].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(load_const_value const& lcv)
{
    if (state.at(lcv.target).alive)
    {
        throw invalid_instruction_transition_error("Attempt to load value into a non-dead slot");
    }

    state[lcv.target].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(make_pointer_to const& mpt)
{
    if (state[mpt.of_index].alive == false)
    {
        throw invalid_instruction_transition_error("Attempt to make pointer to a dead slot");
    }

    if (state[mpt.pointer_index].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[mpt.pointer_index].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(dereference_pointer const& drp)
{
    if (state.at(drp.from_pointer).alive == false)
    {
        throw invalid_instruction_transition_error("Attempt to dereference a dead slot");
    }

    if (state[drp.to_reference].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[drp.to_reference].alive = true;
    state[drp.from_pointer].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(load_from_ref const& lfr)
{
    if (!state.at(lfr.from_reference).alive)
    {
        throw invalid_instruction_transition_error("Attempt to load from a dead slot");
    }

    if (state[lfr.to_value].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[lfr.to_value].alive = true;
    state[lfr.from_reference].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(ret const& ret)
{
}

void quxlang::vmir2::state_engine::apply_internal(int_add const& add)
{
    if (!state[add.a].alive || !state[add.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to add non-alive values");
    }

    if (state[add.result].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[add.result].alive = true;
    state[add.a].alive = false;
    state[add.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(int_sub const& sub)
{
    if (!state[sub.a].alive || !state[sub.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to sub non-alive values");
    }

    if (state[sub.result].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[sub.result].alive = true;
    state[sub.a].alive = false;
    state[sub.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(int_mul const& mul)
{
    if (!state[mul.a].alive || !state[mul.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to mul non-alive values");
    }

    if (state.at(mul.result).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[mul.result].alive = true;
    state[mul.a].alive = false;
    state[mul.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(int_div const& div)
{
    if (!state[div.a].alive || !state[div.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to div non-alive values");
    }

    if (state.at(div.result).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[div.result].alive = true;
    state[div.result].storage_valid = true;
    state[div.result].dtor_enabled = true;

    state[div.a].alive = false;
    state[div.a].storage_valid = false;
    state[div.a].dtor_enabled = false;

    state[div.b].alive = false;
    state[div.b].dtor_enabled = false;
    state[div.b].storage_valid = false;
}

void quxlang::vmir2::state_engine::apply_internal(int_mod const& mod)
{
    if (!state[mod.a].alive || !state[mod.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to mod non-alive values");
    }

    if (state.at(mod.result).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[mod.result].alive = true;
    state[mod.a].alive = false;
    state[mod.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(store_to_ref const& str)
{
    if (!state.at(str.from_value).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store non-alive value");
    }

    if (!state.at(str.to_reference).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a dead slot");
    }

    state[str.to_reference].alive = false;
    state[str.from_value].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(load_const_int const& lci)
{
    output(lci.target);
}

void quxlang::vmir2::state_engine::apply_internal(cmp_eq const& str)
{
    if (!state[str.a].alive || !state[str.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to compare non-alive values");
    }

    if (state[str.result].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[str.result].alive = true;
    state[str.a].alive = false;
    state[str.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(cmp_ge const& str)
{
    if (!state[str.a].alive || !state[str.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to compare non-alive values");
    }

    if (state.at(str.result).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[str.result].alive = true;
    state[str.a].alive = false;
    state[str.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(cmp_lt const& str)
{
    if (!state[str.a].alive || !state[str.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to compare non-alive values");
    }

    if (state[str.result].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[str.result].alive = true;
    state[str.a].alive = false;
    state[str.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(cmp_ne const& str)
{
    if (!state[str.a].alive || !state[str.b].alive)
    {
        throw invalid_instruction_transition_error("Attempt to compare non-alive values");
    }

    if (state.at(str.result).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[str.result].alive = true;
    state[str.a].alive = false;
    state[str.b].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(defer_nontrivial_dtor const& str)
{
    if (!state.at(str.on_value).alive)
    {
        throw invalid_instruction_transition_error("Attempt to defer non-trivial destructor of a dead slot");
    }
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

void quxlang::vmir2::state_engine::apply_internal(copy_reference const& cpr)
{
    if (!state[cpr.from_index].alive)
    {
        throw compiler_bug("this is a bug in cpr or codegen");
    }
    state[cpr.to_index].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(end_lifetime const& elt)
{
    if (!state[elt.of].alive)
    {
        throw compiler_bug("Input not alive");
    }

    state[elt.of].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(pointer_arith const& par)
{
    if (!state[par.from].alive)
    {
        throw invalid_instruction_transition_error("Attempt to perform pointer arithmetic on a dead slot");
    }

    if (!state[par.offset].alive)
    {
        throw invalid_instruction_transition_error("Attempt to use a dead offset slot");
    }

    if (state[par.result].alive)
    {
        throw invalid_instruction_transition_error("Attempt to store pointer arithmetic result into a non-dead slot");
    }

    state[par.result].alive = true;
    state[par.from].alive = false;
    state[par.offset].alive = false;
}

void quxlang::vmir2::state_engine::apply_internal(pointer_diff const& pdf)
{
    consume(pdf.from);
    consume(pdf.to);
    output(pdf.result);
}
void quxlang::vmir2::state_engine::readonly(storage_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("readonly input not alive state");
    }
}
void quxlang::vmir2::state_engine::consume(storage_index idx)
{
    if (!state[idx].alive || !state[idx].storage_valid)
    {
        throw invalid_instruction_transition_error("consume input not alive state");
    }

    state[idx].alive = false;
    state[idx].storage_valid = state[idx].delegate_of.has_value();
}
void quxlang::vmir2::state_engine::output(storage_index idx)
{
    if (state[idx].alive)
    {
        throw invalid_instruction_transition_error("output already set");
    }
    state[idx].alive = true;
    if (state[idx].storage_valid != state[idx].delegate_of.has_value())
    {
        throw invalid_instruction_transition_error("output not valid");
    }
    state[idx].storage_valid = true;
}
