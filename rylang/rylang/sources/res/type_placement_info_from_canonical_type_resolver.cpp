//
// Created by Ryan Nicholl on 10/23/23.
//
#include "rylang/res/type_placement_info_from_canonical_type_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::type_placement_info_from_canonical_type_resolver::process(compiler* c)

{
    canonical_type_reference const& type = m_type;

    if (type.type() == boost::typeindex::type_id< canonical_pointer_type_reference >())
    {
        machine_info m = c->m_machine_info;

        type_placement_info result;
        result.alignment = m.pointer_align;
        result.size = m.pointer_size;

        set_value(result);
        return;
    }
    else if (type.type() == boost::typeindex::type_id< canonical_lookup_chain >())
    {
        auto layout_dp = get_dependency(
            [&]
            {
                return c->lk_class_layout_from_canonical_chain(boost::get< canonical_lookup_chain >(type));
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
    else if (type.type() == boost::typeindex::type_id< integral_keyword_ast >())
    {
        integral_keyword_ast int_kw = boost::get< integral_keyword_ast >(type);

        int sz = 1;
        while (sz*8 < int_kw.size)
        {
          sz *= 2;
        }

        type_placement_info result;
        result.size = sz;
        result.alignment = sz;
        if (c->m_machine_info.max_int_align.has_value())
        {
            result.alignment = std::min(result.alignment, c->m_machine_info.max_int_align.value());
        }
        set_value(result);
        return;
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}
