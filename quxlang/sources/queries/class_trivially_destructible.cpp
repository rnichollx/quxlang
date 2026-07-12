// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_trivially_destructible_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_trivially_destructible_spec > quxlang::class_trivially_destructible_impl(type_symbol input)
{
    class_kind const concrete_kind = co_await rpnx::querygraph::request< class_type_query >(input);
    if (concrete_kind == class_kind::union_)
    {
        union_info const& info = co_await rpnx::querygraph::request< union_info_query >(input);
        for (union_option_info const& option : info.options)
        {
            if (typeis< void_type >(option.type))
            {
                continue;
            }
            if (!info.properties.is_inline || !(co_await rpnx::querygraph::request< class_trivially_destructible_query >(option.type)))
            {
                co_return false;
            }
        }
        co_return true;
    }
    if (concrete_kind == class_kind::variant)
    {
        variant_info const& info = co_await rpnx::querygraph::request< variant_info_query >(input);
        for (type_symbol const& alternative : info.alternatives)
        {
            if (typeis< void_type >(alternative))
            {
                continue;
            }
            if (!info.properties.is_inline || !(co_await rpnx::querygraph::request< class_trivially_destructible_query >(alternative)))
            {
                co_return false;
            }
        }
        co_return true;
    }
    co_return (co_await rpnx::querygraph::request< class_default_dtor_query >(input)).has_value() == false;
}
