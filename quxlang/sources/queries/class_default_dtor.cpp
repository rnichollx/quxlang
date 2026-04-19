// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_default_dtor_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_default_dtor_spec > quxlang::class_default_dtor_impl(type_symbol input)
{
    auto dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};

    initialization_reference init;
    init.initializee = dtor_symbol;
    init.parameters = instatype_from_invotype(invotype{.named{{"THIS", dvalue_slot{input}}}});
    init.adaptations = allowed_adaptations::destination_rebinding;

    auto dtor_inst = co_await rpnx::querygraph::request< functum_initialize_query >(init);

    co_return dtor_inst;
}
