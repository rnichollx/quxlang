//
// Created by Ryan Nicholl on 9/4/2024.
//

#include <quxlang/vmir2/vmir2.hpp>

quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_temporary(type_symbol type)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = std::move(type), .kind = slot_kind::local});
    return slot_id;
}
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_variable(type_symbol type, std::string name)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .name = name, .kind = slot_kind::local});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_binding(storage_index idx, type_symbol type)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .binding_of = idx, .kind = slot_kind::binding});
    return slot_id;
}
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_positional_argument(type_symbol type)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .kind = slot_kind::positional_arg});
    return slot_id;
}
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_named_argument(std::string name, type_symbol type)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .arg_name = name, .kind = slot_kind::named_arg});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_numeric_literal(std::string value)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = numeric_literal_reference{}, .literal_value = value, .kind = slot_kind::literal});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::index_binding(storage_index idx)
{
    auto& slot = slots.at(idx);
    if (slot.kind == vmir2::slot_kind::binding)
    {
        return slot.binding_of.value();
    }
    return idx;
}
quxlang::type_symbol quxlang::vmir2::executable_block_generation_state::current_type(storage_index idx)
{
    if (idx >= slots->slots.size())
    {
        throw std::make_exception_ptr(std::logic_error("Not found"));
    }

    auto& slot = slots->slots.at(idx);
    if (slot.kind == vmir2::slot_kind::binding)
    {
        auto& bound_slot = slots->slots.at(slot.binding_of.value());
        auto bind = bound_type_reference{.carried_type = bound_slot.type, .bound_symbol = slot.type};
        return bind;
    }
    auto type = slots->slots.at(idx).type;
    if (!current_slot_states.at(idx).alive)
    {
        type = create_nslot(type);
    }
    return type;
}
quxlang::vmir2::executable_block_generation_state quxlang::vmir2::executable_block_generation_state::clone_subblock()
{
    executable_block_generation_state copy(*this);
    copy.block.entry_state = current_slot_states;
    return copy;
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::access_field fld)
{
    block.instructions.push_back(fld);
    current_slot_states.at(fld.store_index).alive = true;
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::invoke ivk)
{
    type_symbol invoked_symbol = ivk.what;
    vmir2::invocation_args args = ivk.args;
    std::cout << "emit_invoke(" << quxlang::to_string(invoked_symbol) << ")"
              << " " << quxlang::to_string(args) << std::endl;

    block.instructions.push_back(ivk);

    instanciation_reference inst = as< instanciation_reference >(invoked_symbol);

    for (auto& arg : args.positional)
    {
        type_symbol arg_type = current_type(arg);
        if (typeis< nvalue_slot >(arg_type))
        {
            if (current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
            }
            current_slot_states.at(arg).alive = true;
        }

        else if (typeis< dvalue_slot >(arg_type))
        {
            if (!current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
            }
            current_slot_states.at(arg).alive = false;
        }
        else
        {
            if (!current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
            }

            if (!is_ref(arg_type))
            {
                // In quxlang calling convention, callee is responsible for destroying the argument.
                current_slot_states.at(arg).alive = false;
            }

            // however, references are not destroyed when passed as arguments.
        }
    }
    for (auto& [name, arg] : args.named)
    {
        type_symbol arg_type = current_type(arg);
        if (name == "RETURN")
        {
            current_slot_states.at(arg).alive = true;
            continue;
        }
        type_symbol parameter_type = inst.parameters.named_parameters.at(name);

        if (typeis< nvalue_slot >(parameter_type))
        {
            if (current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
            }
            std::cout << "Setting slot " << arg << " alive" << std::endl;
            current_slot_states.at(arg).alive = true;
        }

        else if (typeis< dvalue_slot >(parameter_type))
        {
            if (!current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
            }
            current_slot_states.at(arg).alive = false;
        }
        else
        {
            if (!current_slot_states.at(arg).alive)
            {
                throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
            }

            if (!is_ref(arg_type))
            {
                // In quxlang calling convention, callee is responsible for destroying the argument.
                // however, references are not destroyed when passed as arguments.
                std::cout << "Setting slot " << arg << " dead" << std::endl;
                current_slot_states.at(arg).alive = false;
            }
        }
    }
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::cast_reference cst)
{
    block.instructions.push_back(cst);
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::make_reference cst)
{
    block.instructions.push_back(cst);
}

bool quxlang::vmir2::executable_block_generation_state::slot_alive(storage_index idx)
{
    return current_slot_states.at(idx).alive;
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_temporary(type_symbol type)
{
    current_slot_states.push_back(slot_state{});
    return slots->create_temporary(type);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_variable(type_symbol type, std::string name)
{
    current_slot_states.push_back(slot_state{});
    return slots->create_variable(type, name);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_binding(storage_index idx, type_symbol type)
{
    current_slot_states.push_back(slot_state{});
    return slots->create_binding(idx, type);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_positional_argument(type_symbol type, std::optional< std::string > label_name)
{
    current_slot_states.push_back(slot_state{});
    return slots->create_positional_argument(type);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_named_argument(std::string interface_name, type_symbol type, std::optional< std::string > label_name)
{
    current_slot_states.push_back(slot_state{});
    return slots->create_named_argument(interface_name, type);
}

quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_numeric_literal(std::string value)
{
    current_slot_states.push_back(slot_state{.alive = true});
    return slots->create_numeric_literal(value);
}

quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::index_binding(storage_index idx)
{
    return slots->index_binding(idx);
}
