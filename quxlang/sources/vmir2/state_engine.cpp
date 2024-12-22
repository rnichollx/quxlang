//
// Created by Ryan Nicholl on 2024-12-21.
//
#include "quxlang/vmir2/state_engine.hpp"
#include "quxlang/exception.hpp"

void quxlang::vmir2::state_engine::apply(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::vm_instruction const& inst)
{
    rpnx::apply_visitor< void >(
        [&](auto const& x)
        {
            apply_internal(state, slot_info, x);
        },
        inst);
}
void quxlang::vmir2::state_engine::apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::load_const_zero const& lcz)
{
    if (state.at(lcz.target).alive)
    {
        throw invalid_instruction_transition_error("Attempt to load zero into a non-dead slot");
    }

    state[lcz.target].alive = true;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::access_field const& acf)
{
    if (!state.at(acf.base_index).alive)
    {
        throw invalid_instruction_transition_error("Attempt to access field of a dead slot");
    }

    if (state.at(acf.store_index).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[acf.store_index].alive = true;
    state[acf.base_index].alive = false;
}
void quxlang::vmir2::state_engine::apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::invoke const& inv)
{
    auto ivk_func_inst = inv.what.get_as< instantiation_type >();

    for (std::size_t index = 0; index < inv.args.positional.size(); index++)
    {
        bool arg_alive = state.at(inv.args.positional[index]).alive;

        auto arg_inst_type = ivk_func_inst.parameters.positional.at(index);

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
        bool arg_alive = state.at(index).alive;

        auto arg_inst_type = ivk_func_inst.parameters.named.at(name);

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
void quxlang::vmir2::state_engine::apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::make_reference const& mrf)
{
    if (state.at(mrf.value_index).alive == false)
    {
        throw invalid_instruction_transition_error("Attempt to make reference to a dead slot");
    }

    if (state.at(mrf.reference_index).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[mrf.reference_index].alive = true;

    // MRF makes a reference without destroying the original value
}

void quxlang::vmir2::state_engine::apply_internal(std::map< std::size_t, slot_state >& state, std::vector< vm_slot > const& slot_info, quxlang::vmir2::cast_reference const& crf)
{
    if (state.at(crf.source_ref_index).alive == false)
    {
        throw invalid_instruction_transition_error("Attempt to cast reference to a dead slot");
    }
    if (state.at(crf.target_ref_index).alive)
    {
        throw invalid_instruction_transition_error("Attempt to store into a non-dead slot");
    }

    state[crf.target_ref_index].alive = true;
    state[crf.source_ref_index].alive = false;
}
