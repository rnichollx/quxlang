// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/type_placement_info_resolver.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/compiler.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_placement_info)
{
    type_symbol const& type = input;
    std::string type_str = to_string(type);

    if (type.template type_is< ptrref_type >())
    {
        output_info m = c->m_output_info;

        type_placement_info result;
        result.alignment = m.pointer_align();
        result.size = m.pointer_size_bytes();

        co_return result;
    }
    else if (type.template type_is< subsymbol >())
    {
        class_layout layout = co_await QUX_CO_DEP(class_layout, (type));

        type_placement_info result;
        result.size = layout.size;
        result.alignment = layout.align;

        co_return result;
    }
    else if (type.template type_is< int_type >())
    {
        int_type int_kw = as< int_type >(type);

        int sz = 1;
        while (sz * 8 < int_kw.bits)
        {
            sz *= 2;
        }

        type_placement_info result;
        result.size = sz;
        result.alignment = sz;
        result.alignment = std::min<std::uint64_t>(result.alignment, c->m_output_info.max_int_align());

        co_return result;
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}