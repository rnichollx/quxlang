// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/type_placement_info_spec.hpp>
#include "quxlang/data/machine.hpp"
#include "quxlang/parsers/parse_int.hpp"


rpnx::querygraph::coroutine< quxlang::type_placement_info_spec > quxlang::type_placement_info_impl(type_symbol input)
{
    type_symbol const& type = input;
    std::string type_str = to_string(type);
    output_info const machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});

    auto expr_u64 = [](expression const& expr) -> std::uint64_t
    {
        if (!expr.type_is< expression_numeric_literal >())
        {
            throw quxlang::semantic_compilation_error("Expected numeric literal in aligned storage type");
        }
        return parsers::str_to_int< std::uint64_t >(expr.get_as< expression_numeric_literal >().value);
    };

    auto nominal_alignment = [&](std::uint64_t byte_count) -> std::uint64_t
    {
        std::uint64_t alignment = 1;
        while (alignment * 2 <= byte_count && alignment * 2 <= machine_info.max_int_align())
        {
            alignment *= 2;
        }
        return alignment;
    };

    if (auto atomic_value_type = atomic_type_argument(type); atomic_value_type.has_value())
    {
        co_return co_await rpnx::querygraph::request< type_placement_info_query >(*atomic_value_type);
    }

    if (type.template type_is< attached_type_reference >())
    {
        attached_type_reference const& attached = as< attached_type_reference >(type);
        if (typeis< void_type >(attached.carrying_type))
        {
            co_return type_placement_info{.size = 0, .alignment = 1};
        }

        co_return co_await rpnx::querygraph::request< type_placement_info_query >(attached.carrying_type);
    }
    else if (type.template type_is< ptrref_type >())
    {
        type_placement_info result;
        result.alignment = machine_info.pointer_align();
        result.size = machine_info.pointer_size_bytes();

        co_return result;
    }
    else if (type.template type_is< subsymbol >() || type.template type_is< instanciation_reference >())
    {
        symbol_kind const kind = co_await rpnx::querygraph::request< symbol_type_query >(type);
        if (kind == symbol_kind::interface_)
        {
            co_return type_placement_info{.size = machine_info.pointer_size_bytes(), .alignment = machine_info.pointer_align()};
        }
        if (kind == symbol_kind::enum_)
        {
            enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(type);
            co_return type_placement_info{.size = info.storage_bytes, .alignment = nominal_alignment(info.storage_bytes)};
        }
        if (kind == symbol_kind::flagset_)
        {
            flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(type);
            co_return type_placement_info{.size = info.storage_bytes, .alignment = nominal_alignment(info.storage_bytes)};
        }

        class_layout layout = co_await rpnx::querygraph::request< class_layout_query >(type);

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
        result.alignment = std::min< std::uint64_t >(result.alignment, machine_info.max_int_align());

        co_return result;
    }
    else if (type.template type_is< float_type >())
    {
        float_type float_kw = as< float_type >(type);

        int sz = 1;
        while (sz * 8 < float_kw.bits)
        {
            sz *= 2;
        }

        type_placement_info result;
        result.size = sz;
        result.alignment = sz;
        result.alignment = std::min< std::uint64_t >(result.alignment, machine_info.max_int_align());

        co_return result;
    }
    else if (type.template type_is< byte_type >() || type.template type_is< bool_type >())
    {
        co_return type_placement_info{.size = 1, .alignment = 1};
    }
    else if (type.template type_is< array_type >())
    {
        auto const& array = as< array_type >(type);
        auto element_placement = co_await rpnx::querygraph::request< type_placement_info_query >(array.element_type);
        auto element_count = expr_u64(array.element_count);
        co_return type_placement_info{.size = element_placement.size * element_count, .alignment = element_placement.alignment};
    }
    else if (type.template type_is< initguard_type >() || type.template type_is< initguard_lock_type >())
    {
        co_return type_placement_info{.size = 8, .alignment = 8};
    }
    else if (type.template type_is< constexpr_proxy >())
    {
        co_return type_placement_info{.size = 0, .alignment = 1};
    }
    else if (type.template type_is< storage >())
    {
        type_placement_info result{.size = 0, .alignment = 1};
        for (auto const& stored_type : as< storage >(type).storable_types)
        {
            auto child = co_await rpnx::querygraph::request< type_placement_info_query >(stored_type);
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
        throw quxlang::compiler_bug("Unimplemented");
    }
}
