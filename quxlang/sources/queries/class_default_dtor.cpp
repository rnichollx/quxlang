// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_default_dtor_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"

#include <set>
#include <string>

rpnx::querygraph::coroutine< quxlang::class_default_dtor_spec > quxlang::class_default_dtor_impl(type_symbol input)
{
    symbol_kind const input_kind = co_await rpnx::querygraph::request< symbol_type_query >(input);
    class_kind const concrete_kind = input_kind == symbol_kind::class_ ? co_await rpnx::querygraph::request< class_type_query >(input) : class_kind::noexist;
    bool uses_split_destructor = false;
    if (concrete_kind == class_kind::struct_)
    {
        std::set< std::string > const tags = co_await rpnx::querygraph::request< struct_tags_query >(input);
        uses_split_destructor = tags.contains(keywords::virtual_polymorphic);
    }
    type_symbol const dtor_symbol = submember{.of = input, .name = uses_split_destructor ? "FULLOBJECT_DESTRUCTOR" : "DESTRUCTOR"};

    initialization_reference init;
    init.initializee = dtor_symbol;
    init.parameters = instatype_from_invotype(invotype{.named{{"THIS", dvalue_slot{input}}}});
    init.adaptations = allowed_adaptations::destination_rebinding;

    auto dtor_inst = co_await rpnx::querygraph::request< functum_initialize_query >(init);

    co_return dtor_inst;
}
