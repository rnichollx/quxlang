//
// Created by Ryan Nicholl on 11/25/23.
//

#include "rylang/res/type_placement_info_from_canonical_type_question.hpp"

namespace rylang
{
    template < typename Graph >
    auto type_placement_info_from_canonical_type_question_f(Graph* g, rylang::qualified_symbol_reference type) -> rpnx::resolver_coroutine< Graph, type_placement_info >
    {
        std::string type_str = to_string(type);

        if (type.type() == boost::typeindex::type_id< pointer_to_reference >())
        {
            machine_info m = g->m_machine_info;

            type_placement_info result;
            result.alignment = m.pointer_align;
            result.size = m.pointer_size;

            co_return result;
        }
        else if (type.type() == boost::typeindex::type_id< subentity_reference >())
        {
            class_layout layout = co_await *g->lk_class_layout_from_canonical_chain(type);

            type_placement_info result;
            result.size = layout.size;
            result.alignment = layout.align;

            co_return result;
        }
        else if (type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
        {
            primitive_type_integer_reference int_kw = boost::get< primitive_type_integer_reference >(type);

            int sz = 1;
            while (sz*8 < int_kw.bits)
            {
                sz *= 2;
            }

            type_placement_info result;
            result.size = sz;
            result.alignment = sz;
            if (g->m_machine_info.max_int_align.has_value())
            {
                result.alignment = std::min(result.alignment, g->m_machine_info.max_int_align.value());
            }
            co_return result;
        }
        else
        {
            throw std::logic_error("Unimplemented");
        }
    }

    template auto type_placement_info_from_canonical_type_question_f< compiler >(compiler* g, rylang::qualified_symbol_reference type) -> rpnx::resolver_coroutine< compiler, type_placement_info >;
} // namespace rylang