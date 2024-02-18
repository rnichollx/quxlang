//
// Created by Ryan Nicholl on 10/23/23.
//
void rylang::type_placement_info_from_canonical_type_resolver::process(compiler* c)

{
    type_symbol const& type = m_type;
    std::string type_str = to_string(type);

    if (type.type() == boost::typeindex::type_id< instance_pointer_type >())
    {
        machine_info m = c->m_machine_info;

        type_placement_info result;
        result.alignment = m.pointer_align();
        result.size = m.pointer_size();

        set_value(result);
        return;
    }
    else if (type.type() == boost::typeindex::type_id< subentity_reference >())
    {
        auto layout_dp = get_dependency(
            [&]
            {
                return c->lk_class_layout_from_canonical_chain(type);
            });

        if (!ready())
            return;

        class_layout layout = layout_dp->get();

        type_placement_info result;
        result.size = layout.size;
        result.alignment = layout.align;

        set_value(result);
        return;
    }
    else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
    {
        primitive_type_integer_reference int_kw = boost::get< primitive_type_integer_reference >(type);

        int sz = 1;
        while (sz * 8 < int_kw.bits)
        {
            sz *= 2;
        }

        type_placement_info result;
        result.size = sz;
        result.alignment = sz;
        result.alignment = std::min(result.alignment, c->m_machine_info.max_int_align());

        set_value(result);
        return;
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}
