// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/sintpointer_type_spec.hpp>

#include <quxlang/vmir2/vmir2.hpp>

rpnx::querygraph::coroutine< quxlang::sintpointer_type_spec > quxlang::sintpointer_type_impl(std::monostate input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});

    std::uint64_t const pointer_carrier_bits = cpu_is_layoutless(machine_info.cpu_type) ? 64 : machine_info.pointer_size_bytes() * 8;
    co_return int_type{.bits = pointer_carrier_bits, .has_sign = true};
}
