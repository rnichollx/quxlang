// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com

#include "quxlang/vmir2/assembly.hpp"
#include "rpnx/value.hpp"

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
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::to_bool tb)
{
    block.instructions.push_back(tb);
    current_slot_states[tb.from].alive = false;
    current_slot_states[tb.to].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::to_bool_not tbn)
{
    block.instructions.push_back(tbn);
    current_slot_states[tbn.from].alive = false;
    current_slot_states[tbn.to].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::predecrement pdec)
{
    block.instructions.push_back(pdec);
    current_slot_states[pdec.target].alive = false;
    current_slot_states[pdec.target2].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::decrement dec)
{
    block.instructions.push_back(dec);
    current_slot_states[dec.target].alive = false;
    current_slot_states[dec.oldval].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::increment inc)
{
    block.instructions.push_back(inc);
    current_slot_states[inc.value].alive = false;
    current_slot_states[inc.result].alive = true;
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::preincrement pinc)
{
    block.instructions.push_back(pinc);
    current_slot_states[pinc.target].alive = false;
    current_slot_states[pinc.target2].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::access_field fld)
{
    block.instructions.push_back(fld);
    current_slot_states[fld.store_index].alive = true;
    current_slot_states[fld.base_index].alive = false;
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::access_array aca)
{
    block.instructions.push_back(aca);
    current_slot_states[aca.store_index].alive = true;
    current_slot_states[aca.base_index].alive = false;
    current_slot_states[aca.index_index].alive = false;
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

            // In quxlang calling convention, callee is responsible for destroying the argument.
            current_slot_states.at(arg).alive = false;
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
        type_symbol parameter_type = inst.params.named.at(name);

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

            // In quxlang calling convention, callee is responsible for destroying the argument.
            // however, references are not destroyed when passed as arguments.
            current_slot_states.at(arg).alive = false;
        }
    }
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::cast_reference cst)
{
    block.instructions.push_back(cst);

    current_slot_states[cst.source_ref_index].alive = false;
    current_slot_states[cst.target_ref_index].alive = true;
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::make_reference cst)
{
    current_slot_states[cst.reference_index].alive = true;
    block.instructions.push_back(cst);
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::copy_reference cpr)
{
    current_slot_states[cpr.to_index].alive = true;
    block.instructions.push_back(cpr);
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
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::constexpr_set_result csr)
{
    block.instructions.push_back(csr);
    current_slot_states.at(csr.target).alive = false;
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::load_const_value lcv)
{
    current_slot_states.at(lcv.target).alive = true;
    block.instructions.push_back(lcv);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::load_const_int lci)
{
    current_slot_states[lci.target].alive = true;
    block.instructions.push_back(lci);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::make_pointer_to mpt)
{
    current_slot_states[mpt.pointer_index].alive = true;
    block.instructions.push_back(mpt);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::dereference_pointer drp)
{
    current_slot_states[drp.from_pointer].alive = false;
    current_slot_states[drp.to_reference].alive = true;
    block.instructions.push_back(drp);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::load_from_ref lfp)
{
    assert(current_slot_states[lfp.from_reference].alive);
    current_slot_states[lfp.from_reference].alive = false;
    current_slot_states[lfp.to_value].alive = true;
    block.instructions.push_back(lfp);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::store_to_ref lfp)
{
    assert(current_slot_states[lfp.from_value].alive == true);
    assert(current_slot_states[lfp.to_reference].alive == true);
    current_slot_states[lfp.from_value].alive = false;
    current_slot_states[lfp.to_reference].alive = false;
    block.instructions.push_back(lfp);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::int_add add)
{
    assert(current_slot_states[add.a].alive);
    assert(current_slot_states[add.b].alive);
    current_slot_states[add.a].alive = false;
    current_slot_states[add.b].alive = false;
    current_slot_states[add.result].alive = true;
    block.instructions.push_back(add);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::int_sub sub)
{
    assert(current_slot_states[sub.a].alive);
    assert(current_slot_states[sub.b].alive);
    current_slot_states[sub.a].alive = false;
    current_slot_states[sub.b].alive = false;
    current_slot_states[sub.result].alive = true;
    block.instructions.push_back(sub);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::int_mul mul)
{
    assert(current_slot_states[mul.a].alive);
    assert(current_slot_states[mul.b].alive);
    current_slot_states[mul.a].alive = false;
    current_slot_states[mul.b].alive = false;
    current_slot_states[mul.result].alive = true;
    block.instructions.push_back(mul);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::int_div div)
{
    assert(current_slot_states[div.a].alive);
    assert(current_slot_states[div.b].alive);
    current_slot_states[div.a].alive = false;
    current_slot_states[div.b].alive = false;
    current_slot_states[div.result].alive = true;
    block.instructions.push_back(div);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::int_mod mod)
{
    assert(current_slot_states[mod.a].alive);
    assert(current_slot_states[mod.b].alive);
    current_slot_states[mod.a].alive = false;
    current_slot_states[mod.b].alive = false;
    current_slot_states[mod.result].alive = true;
    block.instructions.push_back(mod);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::load_const_zero lcz)
{
    current_slot_states[lcz.target].alive = true;
    block.instructions.push_back(lcz);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::cmp_eq ceq)
{
    assert(current_slot_states[ceq.a].alive);
    assert(current_slot_states[ceq.b].alive);
    current_slot_states[ceq.a].alive = false;
    current_slot_states[ceq.b].alive = false;
    current_slot_states[ceq.result].alive = true;
    block.instructions.push_back(ceq);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::cmp_ne cne)
{
    assert(current_slot_states[cne.a].alive);
    assert(current_slot_states[cne.b].alive);
    current_slot_states[cne.a].alive = false;
    current_slot_states[cne.b].alive = false;
    current_slot_states[cne.result].alive = true;
    block.instructions.push_back(cne);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::cmp_lt clt)
{
    assert(current_slot_states[clt.a].alive);
    assert(current_slot_states[clt.b].alive);
    current_slot_states[clt.a].alive = false;
    current_slot_states[clt.b].alive = false;
    current_slot_states[clt.result].alive = true;
    block.instructions.push_back(clt);
}

void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::cmp_ge cge)
{
    assert(current_slot_states[cge.a].alive);
    assert(current_slot_states[cge.b].alive);
    current_slot_states[cge.a].alive = false;
    current_slot_states[cge.b].alive = false;
    current_slot_states[cge.result].alive = true;
    block.instructions.push_back(cge);
}
void quxlang::vmir2::executable_block_generation_state::emit(quxlang::vmir2::defer_nontrivial_dtor dntd)
{
    block.instructions.push_back(dntd);
    current_slot_states[dntd.on_value].nontrivial_dtor = dtor_spec{.func = dntd.func, .args = dntd.args};
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::struct_delegate_new sdn)
{
    assert(current_slot_states[sdn.on_value].alive == false);

    block.instructions.push_back(sdn);

    current_slot_states[sdn.on_value].delegates = invocation_args{};
    current_slot_states[sdn.on_value].alive = true;

    for (auto const& arg : sdn.fields.positional)
    {
        assert(current_slot_states[arg].alive == false);
        current_slot_states[arg].delegate_of = sdn.on_value;

        current_slot_states[sdn.on_value].delegates.value().positional.push_back(arg);
    }

    for (auto const& [name, arg] : sdn.fields.named)
    {
        current_slot_states[arg].delegate_of = sdn.on_value;
        current_slot_states[sdn.on_value].delegates.value().named[name] = arg;
    }
}
void quxlang::vmir2::executable_block_generation_state::emit(vmir2::end_lifetime elt)
{
    current_slot_states[elt.of] = {};
}

void quxlang::vmir2::executable_block_generation_state::emit(vmir2::struct_complete_new scn)
{
    assert(current_slot_states[scn.on_value].alive == true);

    // TODO
    // block.instructions.push_back(scn);

    current_slot_states[scn.on_value].delegates = invocation_args{};
    current_slot_states[scn.on_value].alive = true;

    throw rpnx::unimplemented();
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
