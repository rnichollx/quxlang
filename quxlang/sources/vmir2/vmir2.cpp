// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

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
    if (!current_slot_states[idx].alive && !typeis< nvalue_slot >(type))
    {
        type = create_nslot(type);
    }
    else if (typeis< nvalue_slot >(type) && current_slot_states[idx].alive)
    {
        type = type_symbol(as< nvalue_slot >(type).target);
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
    current_slot_states[fld.store_index].alive = true;
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
        type_symbol parameter_type = inst.parameters.named.at(name);

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
    auto idx = slots->create_temporary(type);
    return idx;
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_variable(type_symbol type, std::string name)
{
    return slots->create_variable(type, name);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_binding(storage_index idx, type_symbol type)
{
    return slots->create_binding(idx, type);
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_positional_argument(type_symbol type, std::optional< std::string > label_name)
{
    auto idx = slots->create_positional_argument(type);
    if (!typeis< nvalue_slot >(type))
    {
        current_slot_states[idx].alive = true;
    }
    return idx;
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_named_argument(std::string interface_name, type_symbol type, std::optional< std::string > label_name)
{

    auto idx = slots->create_named_argument(interface_name, type);
    if (!typeis< nvalue_slot >(type))
    {
        current_slot_states[idx].alive = true;
    }
    if (interface_name == "RETURN" || interface_name == "THIS")
    {
        // Always set these, they have special handling
        named_references[interface_name] = idx;
    }
    named_references[label_name.value_or(interface_name)] = idx;
    return idx;
}

quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_numeric_literal(std::string value)
{
    auto idx = slots->create_numeric_literal(value);
    current_slot_states[idx].alive = true;
    return idx;
}

quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::index_binding(storage_index idx)
{
    return slots->index_binding(idx);
}
std::optional< quxlang::vmir2::storage_index > quxlang::vmir2::executable_block_generation_state::local_lookup(std::string name)
{
    for (std::size_t i = 0; i < slots->slots.size(); i++)
    {
        if (slots->slots[i].name.has_value() && slots->slots[i].name.value() == name && current_slot_states[i].alive)
        {
            return i;
        }
    }
    return std::nullopt;
}
void quxlang::vmir2::frame_generation_state::generate_jump(std::size_t from, std::size_t to)
{
    if (block(from).block.terminator.has_value())
    {
        throw std::logic_error("Cannot jump from a block that already has a terminator");
    }

    block(from).block.terminator = vmir2::jump{.target = to};
    // TODO: Check value lifetimes here.
}
void quxlang::vmir2::frame_generation_state::generate_branch(std::size_t condition, std::size_t from, std::size_t true_branch, std::size_t false_branch)
{
    if (block(from).block.terminator.has_value())
    {
        throw std::logic_error("Cannot branch from a block that already has a terminator");
    }
    block(from).block.terminator = vmir2::branch{.condition = condition, .target_true = true_branch, .target_false = false_branch};
    // TODO: Check value lifetimes here.
}
void quxlang::vmir2::frame_generation_state::generate_return(std::size_t from)
{
    block(from).block.terminator = vmir2::ret{};
    // TODO: validate lifetimes are as expected.
}
std::size_t quxlang::vmir2::frame_generation_state::generate_entry_block()
{
    // Either the initial (argument) block, or th
    if (!block_states.empty())
    {
        throw std::logic_error("Cannot generate entry block when there are already blocks");
    }
    block_states.emplace_back(&slots);

    entry_block_opt = 0;

    return 0;
}
std::size_t quxlang::vmir2::frame_generation_state::generate_subblock(std::size_t of)
{
    std::size_t block_id = block_states.size();
    block_states.push_back(block(of).clone_subblock());
    // TODO: check states valid.

    return block_id;
}
std::size_t quxlang::vmir2::frame_generation_state::entry_block_id()
{
    return entry_block_opt.value();
}
quxlang::vmir2::executable_block_generation_state& quxlang::vmir2::frame_generation_state::entry_block()
{
    return block_states.at(entry_block_id());
}
quxlang::vmir2::executable_block_generation_state& quxlang::vmir2::frame_generation_state::block(std::size_t id)
{
    return block_states.at(id);
}
std::optional< quxlang::vmir2::storage_index > quxlang::vmir2::frame_generation_state::lookup(std::size_t block_id, std::string name)
{
    auto it = block(block_id).named_references.find(name);
    if (it != block(block_id).named_references.end())
    {
        return it->second;
    }

    // TODO: Replace this logic
    for (std::size_t i = 0; i < slots.slots.size(); i++)
    {
        auto& slot = slots.slots[i];
        if (slot.kind == slot_kind::named_arg || slot.kind == slot_kind::positional_arg)
        {
            if (slot.name.has_value() && slot.name.value() == name || (!slot.name.has_value() && slot.arg_name.has_value() && slot.arg_name.value() == name))
            {
                return i;
            }
        }
    }
    return std::nullopt;
}
quxlang::vmir2::functanoid_routine2 quxlang::vmir2::frame_generation_state::get_result()
{
    quxlang::vmir2::functanoid_routine2 result;
    result.slots = slots.slots;
    result.blocks.reserve(block_states.size());
    for (auto& block : block_states)
    {
        result.blocks.push_back(block.block);
    }
    return result;
}
