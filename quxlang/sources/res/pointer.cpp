//
// Created by rnicholl on 4/28/25.
//

#include "quxlang/res/pointer.hpp"

#include "quxlang/compiler.hpp"
#include "quxlang/data/type_symbol.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(uintpointer_type)
{
    auto machin_info = c->m_output_info;

    auto ptr_size = machin_info.pointer_size();
    // ptr_size is size in bytes, so we need to multiply by 8 to get bits
    auto bits = ptr_size * 8;

    co_return int_type{.bits = bits, .has_sign = false};
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(sintpointer_type)
{
    auto machin_info = c->m_output_info;

    auto ptr_size = machin_info.pointer_size();
    // ptr_size is size in bytes, so we need to multiply by 8 to get bits
    auto bits = ptr_size * 8;

    co_return int_type{.bits = bits, .has_sign = true};
}