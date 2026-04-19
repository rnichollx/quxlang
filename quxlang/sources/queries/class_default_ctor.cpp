// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/class_default_ctor_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::class_default_ctor_spec > quxlang::class_default_ctor_impl(type_symbol input)
{
    auto ctor_symbol = submember{.of = input, .name = "CONSTRUCTOR"};

    initialization_reference init;
    init.initializee = ctor_symbol;
    init.parameters = invotype{.named{{"THIS", nvalue_slot{input}}}};
    init.adaptations = allowed_adaptations::destination_rebinding;

    auto ctor_inst = co_await rpnx::querygraph::request< functum_initialize_query >(init);

    co_return ctor_inst;
}
