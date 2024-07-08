//
// Created by Ryan Nicholl on 10/23/23.
//
#include "quxlang/res/type_placement_info_from_canonical_type_resolver.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_placement_info_from_canonical_type)
{
    type_symbol const& type = input;
    std::string type_str = to_string(type);

    if (type.type() == boost::typeindex::type_id< instance_pointer_type >())
    {
        output_info m = c->m_machine_info;

        type_placement_info result;
        result.alignment = m.pointer_align();
        result.size = m.pointer_size();

        co_return result;
    }
    else if (type.type() == boost::typeindex::type_id< subentity_reference >())
    {
        class_layout layout = co_await QUX_CO_DEP(class_layout, (type));

        type_placement_info result;
        result.size = layout.size;
        result.alignment = layout.align;

        co_return result;
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
    {
        primitive_type_integer_reference int_kw = as< primitive_type_integer_reference >(type);

        int sz = 1;
        while (sz * 8 < int_kw.bits)
        {
            sz *= 2;
        }

        type_placement_info result;
        result.size = sz;
        result.alignment = sz;
        result.alignment = std::min(result.alignment, c->m_machine_info.max_int_align());

        co_return result;
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}