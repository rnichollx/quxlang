// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/type_placement_info_resolver.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/data/machine.hpp"
#include "quxlang/parsers/parse_int.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_placement_info)
{
    type_symbol const& type = input;
    std::string type_str = to_string(type);

    auto expr_u64 = [](expression const& expr) -> std::uint64_t
    {
        if (!expr.type_is< expression_numeric_literal >())
        {
            throw std::logic_error("Expected numeric literal in aligned storage type");
        }
        return parsers::str_to_int< std::uint64_t >(expr.get_as< expression_numeric_literal >().value);
    };

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
        result.alignment = std::min< std::uint64_t >(result.alignment, c->m_output_info.max_int_align());

        co_return result;
    }
    else if (type.template type_is< byte_type >() || type.template type_is< bool_type >())
    {
        co_return type_placement_info{.size = 1, .alignment = 1};
    }
    else if (type.template type_is< storage >())
    {
        type_placement_info result{.size = 0, .alignment = 1};
        for (auto const& stored_type : as< storage >(type).storable_types)
        {
            auto child = co_await QUX_CO_DEP(type_placement_info, (stored_type));
            result.size = std::max< std::uint64_t >(result.size, child.size);
            result.alignment = std::max< std::uint64_t >(result.alignment, child.alignment);
        }
        co_return result;
    }
    else if (type.template type_is< aligned_storage >())
    {
        co_return type_placement_info{.size = expr_u64(as< aligned_storage >(type).size), .alignment = expr_u64(as< aligned_storage >(type).align)};
    }
    else
    {
        throw std::logic_error("Unimplemented");
    }
}
