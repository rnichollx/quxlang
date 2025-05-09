//
// Created by Ryan Nicholl on 2024-12-21.
//
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/exception.hpp"

void quxlang::vmir2::state_engine::apply(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::vm_instruction const& inst)
{
    rpnx::apply_visitor< void >(
        [&](auto const& x)
        {
            apply_internal(state, slot_info, x);
        },
        inst);
}
void quxlang::vmir2::state_engine::apply_entry(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info)
{
    for (std::size_t i = 0; i < slot_info.size(); i++)
    {
        auto const& slot = slot_info[i];

        if (slot.kind == slot_kind::named_arg || slot.kind == slot_kind::positional_arg)
        {
            if (!typeis< nvalue_slot >(slot.type))
            {
                state[i].alive = true;
            }
            else
            {
                state[i].alive = false;
            }
        }
    }
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, increment const& tb)
{
    if (!state.at(tb.value).alive)
    {
        throw invalid_instruction_transition_error("Attempt to increment a dead slot");
    }

    state[tb.value].alive = false;
    state[tb.result].alive = true;
}


void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, decrement const& acf)
{
    // TODO: State checks
    state[acf.target].alive = false;
    state[acf.oldval].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, preincrement const& pinc)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, predecrement const& pdec)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, to_bool const& tb)
{
    if (!state.at(tb.from).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, to_bool_not const& tbn)
{
    if (!state.at(tbn.from).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::load_const_zero const& lcz)
{
    if (state[lcz.target].alive)
    {
        throw invalid_instruction_transition_error("Attempt to load zero into a non-dead slot");
    }

    state[lcz.target].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::access_field const& acf)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, access_array const& aca)
{

    // TODO: add state checks

    state[aca.base_index].alive = false;
    state[aca.index_index].alive = false;
    state[aca.store_index].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::invoke const& inv)
{
    auto ivk_func_inst = inv.what.get_as< instanciation_reference >();

    for (std::size_t index = 0; index < inv.args.positional.size(); index++)
    {
        bool arg_alive = state.at(inv.args.positional[index]).alive;

        auto arg_inst_type = ivk_func_inst.params.positional.at(index);

        if (arg_inst_type.template type_is< nvalue_slot >())
        {
            if (arg_alive)
            {
                throw invalid_instruction_transition_error("calling NEW& with existing value");
            }

            state[inv.args.positional[index]].alive = true;
        }
        else
        {
            if (!arg_alive)
            {
                throw invalid_instruction_transition_error("calling func with non-alive value");
            }

            state[inv.args.positional[index]].alive = false;
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
            if (arg_alive)
            {
                throw invalid_instruction_transition_error("calling NEW& with existing value");
            }

            state[index].alive = true;
        }
        else
        {
            if (!arg_alive)
            {
                throw invalid_instruction_transition_error("calling func with non-alive value");
            }

            state[index].alive = false;
        }
    }
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::make_reference const& mrf)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cast_reference const& crf)
{
    if (state.at(crf.source_ref_index).alive == false)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::constexpr_set_result const& csr)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::load_const_value const& lcv)
{
    if (state.at(lcv.target).alive)
    {
        throw invalid_instruction_transition_error("Attempt to load value into a non-dead slot");
    }

    state[lcv.target].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::make_pointer_to const& mpt)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::dereference_pointer const& drp)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::load_from_ref const& lfr)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::ret const& ret)
{
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::int_add const& add)
{
    if (!state.at(add.a).alive || !state.at(add.b).alive)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::int_sub const& sub)
{
    if (!state.at(sub.a).alive || !state.at(sub.b).alive)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::int_mul const& mul)
{
    if (!state.at(mul.a).alive || !state.at(mul.b).alive)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::int_div const& div)
{
    if (!state.at(div.a).alive || !state.at(div.b).alive)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::int_mod const& mod)
{
    if (!state.at(mod.a).alive || !state.at(mod.b).alive)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::store_to_ref const& str)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::load_const_int const& str)
{
    if (state[str.target].alive)
    {
        throw invalid_instruction_transition_error("Attempt to load int into a non-dead slot");
    }

    state[str.target].alive = true;
}

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cmp_eq const& str)
{
    if (!state.at(str.a).alive || !state.at(str.b).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cmp_ge const& str)
{
    if (!state.at(str.a).alive || !state.at(str.b).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cmp_lt const& str)
{
    if (!state.at(str.a).alive || !state.at(str.b).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cmp_ne const& str)
{
    if (!state.at(str.a).alive || !state.at(str.b).alive)
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

void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::defer_nontrivial_dtor const& str)
{
    if (!state.at(str.on_value).alive)
    {
        throw invalid_instruction_transition_error("Attempt to defer non-trivial destructor of a dead slot");
    }
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, struct_delegate_new const& dlg)
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
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, copy_reference const& cpr)
{
    if (!state.at(cpr.from_index).alive)
    {
        throw compiler_bug("this is a bug in cpr or codegen");
    }
    state[cpr.to_index].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< vmir2::storage_index, slot_state >& state, std::vector< vm_slot > const& slot_info, end_lifetime const& elt)
{
    if (!state.at(elt.of).alive)
    {
        throw compiler_bug("Input not alive");
    }

    state[elt.of].alive = false;
}