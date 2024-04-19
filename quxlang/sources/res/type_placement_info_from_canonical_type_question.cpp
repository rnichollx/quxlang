//
// Created by Ryan Nicholl on 11/25/23.
//

#include "quxlang/res/type_placement_info_from_canonical_type_question.hpp"
#include "quxlang/compiler.hpp"

namespace quxlang
{
    template < typename Graph >
    auto general_int_4_returner() -> rpnx::general_coroutine< Graph, int >
    {
        co_return 4;
    }

    template < typename Graph >
    auto get_class_layout_from_canonical_chain(Graph* g, type_symbol cls) -> rpnx::general_coroutine< Graph, class_layout >
    {
        //  int four = co_await general_int_4_returner< Graph >();
        co_return co_await *g->lk_class_layout_from_canonical_chain(cls);
    }

    template < typename Graph >
    auto type_placement_info_from_canonical_type_question_f(Graph* g, quxlang::type_symbol type) -> rpnx::resolver_coroutine< Graph, type_placement_info >
    {
        std::string type_str = to_string(type);

        int four = co_await general_int_4_returner< Graph >();

        if (type.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            output_info m = g->m_machine_info;

            type_placement_info result;
            result.alignment = m.pointer_align();
            result.size = m.pointer_size();

            co_return result;
        }
        else if (typeis< subentity_reference >(type) || typeis< instanciation_reference >(type))
        {
            class_layout layout = co_await get_class_layout_from_canonical_chain(g, type);

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
            result.alignment = std::min(result.alignment, g->m_machine_info.max_int_align());
            co_return result;
        }
        else
        {
            throw std::logic_error("Unimplemented");
        }

        rpnx::unimplemented();
    }

    template auto type_placement_info_from_canonical_type_question_f< compiler >(compiler* g, quxlang::type_symbol type) -> rpnx::resolver_coroutine< compiler, type_placement_info >;
} // namespace quxlang