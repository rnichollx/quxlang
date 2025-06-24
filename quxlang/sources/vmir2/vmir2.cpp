#include "quxlang/vmir2/assembly.hpp"
#include "rpnx/value.hpp"

#include <quxlang/ast2/source_location.hpp>
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
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_positional_argument(type_symbol type, std::optional< std::string > name)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .name = name, .kind = slot_kind::positional_arg});
    return slot_id;
}
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_named_argument(std::string apiname, type_symbol type, std::optional< std::string > varname)
{
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = type, .name = varname.value_or(apiname), .arg_name = apiname, .kind = slot_kind::named_arg});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_numeric_literal(std::string value)
{
    // TODO Check if this literal already exists
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = numeric_literal_reference{}, .literal_value = value, .kind = slot_kind::literal});
    return slot_id;
}
quxlang::vmir2::storage_index quxlang::vmir2::slot_generation_state::create_bool_literal(bool value)
{
    // TODO Check if this literal already exists.
    storage_index slot_id = slots.size();
    slots.push_back(vm_slot{.type = bool_type{}, .literal_value = value ? "TRUE" : "FALSE", .kind = slot_kind::literal});
    return slot_id;
}

// Definition for slot_generation_state constructor
quxlang::vmir2::slot_generation_state::slot_generation_state()
{
    slots.push_back(vmir2::vm_slot{.type = void_type{}, .name = "VOID", .literal_value = "VOID", .kind = vmir2::slot_kind::literal});
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
        throw std::logic_error("Not found");
    }

    auto& slot = slots->slots.at(idx);
    if (slot.kind == vmir2::slot_kind::binding)
    {
        auto& bound_slot = slots->slots.at(slot.binding_of.value());
        auto bind = bound_type_reference{.carried_type = bound_slot.type, .bound_symbol = slot.type};
        return bind;
    }
    auto type = slots->slots.at(idx).type;
    if (typeis< dvalue_slot >(type) && !current_slot_states[idx].alive)
    {
        functanoid_routine2 fakefunc;
        throw compiler_bug("Dvalue should be alive on entry");
    }
    if (!current_slot_states[idx].alive && !typeis< nvalue_slot >(type) && !typeis< dvalue_slot >(type))
    {
        type = create_nslot(type);
    }
    else if (typeis< nvalue_slot >(type) && current_slot_states[idx].alive)
    {
        type = type_symbol(as< nvalue_slot >(type).target);
    }
    else if (typeis< dvalue_slot >(type))
    {
        type = make_mref(type_symbol(as< dvalue_slot >(type).target));
    }
    return type;
}
quxlang::vmir2::executable_block_generation_state quxlang::vmir2::executable_block_generation_state::clone_subblock()
{
    executable_block_generation_state copy(*this);
    copy.block.instructions.clear();
    copy.block.dbg_name.reset();
    copy.block.entry_state = current_slot_states;
    return copy;
}

// Definition for executable_block_generation_state::emit for vm_instruction
void quxlang::vmir2::executable_block_generation_state::emit(vm_instruction inst)
{
    block.instructions.push_back(inst);
    state_engine(current_slot_states, slots->slots).apply(inst);
}

// Definition for executable_block_generation_state::emit for vm_terminator
void quxlang::vmir2::executable_block_generation_state::emit(vm_terminator term)
{
    block.terminator = term;
    // No state update needed for terminators.
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
    auto idx = slots->create_positional_argument(type, label_name);
    if (!typeis< nvalue_slot >(type))
    {
        current_slot_states[idx].alive = true;
        current_slot_states[idx].storage_valid = true;
    }
    if (label_name.has_value())
    {
        named_references[label_name.value()] = idx;
    }
    return idx;
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_named_argument(std::string interface_name, type_symbol type, std::optional< std::string > label_name)
{

    auto idx = slots->create_named_argument(interface_name, type, label_name);
    if (!typeis< nvalue_slot >(type))
    {
        current_slot_states[idx].alive = true;
        current_slot_states[idx].storage_valid = true;
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
    current_slot_states[idx].storage_valid = true;
    // TODO: This is not actually a storable object, but we need to handle this for now because there
    // is not separation between codegen slots and IR slots.
    return idx;
}
quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::create_bool_literal(bool value)
{
    auto idx = slots->create_bool_literal(value);
    current_slot_states[idx].alive = true;
    current_slot_states[idx].storage_valid = true;
    // TODO: This is not actually a storable object, but we need to handle this for now because there
    // is not separation between codegen slots and IR slots.
    return idx;
}

quxlang::vmir2::storage_index quxlang::vmir2::executable_block_generation_state::index_binding(storage_index idx)
{
    return slots->index_binding(idx);
}
std::optional< quxlang::vmir2::storage_index > quxlang::vmir2::executable_block_generation_state::local_lookup(std::string name)
{

    if (this->named_references.contains(name))
    {
        return this->named_references.at(name);
    }

    for (std::size_t i = 0; i < slots->slots.size(); i++)
    {
        if (slots->slots[i].name.has_value() && slots->slots[i].name.value() == name && current_slot_states[i].alive)
        {
            throw compiler_bug("this shouldn't happen now");
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
std::size_t quxlang::vmir2::frame_generation_state::generate_subblock(std::size_t of, std::string dbg_str)
{
    std::size_t block_id = block_states.size();
    block_states.push_back(block(of).clone_subblock());
    // TODO: check states valid.

    block(block_id).block.dbg_name = dbg_str;

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
            if ((slot.name.has_value() && slot.name.value() == name) || (!slot.name.has_value() && slot.arg_name.has_value() && slot.arg_name.value() == name))
            {
                return i;
            }
        }
    }
    return std::nullopt;
}

// Definition for frame_generation_state::has_terminator
bool quxlang::vmir2::frame_generation_state::has_terminator(std::size_t block)
{
    return block_states[block].block.terminator.has_value();
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
    result.non_trivial_dtors = non_trivial_dtors;
    return result;
}
