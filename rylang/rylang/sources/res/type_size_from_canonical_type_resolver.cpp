//
// Created by Ryan Nicholl on 10/20/23.
//
#include "rylang/res/type_size_from_canonical_type_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::type_size_from_canonical_type_resolver::process(compiler* c)
{
    canonical_type_reference const& type = m_type;

    if (type.type() == boost::typeindex::type_id< canonical_pointer_type_reference >())
    {
        machine_info m = c->m_machine_info;

        set_value(m.pointer_size);
    }
    else if (type.type() == boost::typeindex::type_id< canonical_lookup_chain >())
    {
        auto result_dp = get_dependency(
            [&]
            {
                return c->lk_class_size_from_canonical_lookup_chain(boost::get< canonical_lookup_chain >(type));
            });

        if (!ready())
            return;

        set_value(result_dp->get());
    }
    else if (type.type() == boost::typeindex::type_id< integral_keyword_ast >())
    {
        integral_keyword_ast int_kw = boost::get< integral_keyword_ast >(type);

        int sz = 1;
        while (sz*8 < int_kw.size)
        {
          sz *= 2;
        }

        set_value(sz);
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}
