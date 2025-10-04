// Copyright 2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/pointer.hpp"

#include "quxlang/compiler.hpp"

#include <quxlang/vmir2/vmir2.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(uintpointer_type)
{
    auto machine_info = c->m_output_info;

    co_return int_type{.bits = machine_info.pointer_size_bytes() * 8, .has_sign = false};
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(sintpointer_type)
{
    auto machine_info = c->m_output_info;

    co_return int_type{.bits = machine_info.pointer_size_bytes() * 8, .has_sign = true};
}