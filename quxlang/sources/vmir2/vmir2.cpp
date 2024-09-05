//
// Created by Ryan Nicholl on 9/4/2024.
//

#include <quxlang/vmir2/vmir2.hpp>

quxlang::vmir2::storage_index quxlang::vmir2::create_temporary(frame& frame, type_symbol type)
{
    storage_index slot_id = frame.slots.size();
    frame.slots.push_back(vm_slot{.type = type, .kind = slot_kind::local});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::create_variable(frame& frame, type_symbol type, std::string name)
{
    storage_index slot_id = frame.slots.size();
    frame.slots.push_back(vm_slot{.type = type, .name = name, .kind = slot_kind::local});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::create_binding(frame& frame, storage_index idx, type_symbol type)
{
    storage_index slot_id = frame.slots.size();
    frame.slots.push_back(vm_slot{.type = type, .binding_of = idx, .kind = slot_kind::binding });
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::create_argument(frame& frame, type_symbol type)
{
    storage_index slot_id = frame.slots.size();
    frame.slots.push_back(vm_slot{.type = type, .kind = slot_kind::arg});
    return slot_id;
}

quxlang::vmir2::storage_index quxlang::vmir2::create_numeric_literal(frame& frame, std::string value)
{
    storage_index slot_id = frame.slots.size();
    frame.slots.push_back(vm_slot{.type = numeric_literal_reference{}, .literal_value = value, .kind = slot_kind::literal});
    return slot_id;
}
