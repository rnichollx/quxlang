// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_default_dtor_exists_spec.hpp>
#include "quxlang/data/expression.hpp"
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/value.hpp"


rpnx::querygraph::coroutine< quxlang::user_default_dtor_exists_spec > quxlang::user_default_dtor_exists_impl(type_symbol input)
{
    auto dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};

    auto user_defined_dtor = co_await rpnx::querygraph::query_request< functum_overloads_query >(dtor_symbol);

    auto dtor_call_type = invotype{.named{{"THIS", dvalue_slot{input}}}};
    auto dtor_default_intertype = intertype{.named{{"THIS", argif{.type = dvalue_slot{input}}}}};

    // Look through destructors to find default destructor

    std::optional< type_symbol > class_default_dtor;

    for (auto& ol : user_defined_dtor)
    {
        auto candidate = co_await rpnx::querygraph::query_request< function_ensig_init_with_query >({.ensig = ol, .params = dtor_call_type, .adaptations = allowed_adaptations::none});

        if (candidate)
        {
            instanciation_reference inst;
            inst.temploid = temploid_reference{.templexoid = dtor_symbol, .which = ol};
            inst.params = *candidate;
            co_return true;
        }
    }

    co_return false;
}
