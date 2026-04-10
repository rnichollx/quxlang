// Copyright 2025-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/uintpointer_type_spec.hpp>

#include <quxlang/vmir2/vmir2.hpp>


rpnx::querygraph::coroutine< quxlang::uintpointer_type_spec > quxlang::uintpointer_type_impl(std::monostate input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(std::monostate{});

    co_return int_type{.bits = machine_info.pointer_size_bytes() * 8, .has_sign = false};
}
