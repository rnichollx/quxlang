// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_default_ctor_exists_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::user_default_ctor_exists_spec > quxlang::user_default_ctor_exists_impl(type_symbol input)
{
    auto ctor_symbol = submember{.of = input, .name = "CONSTRUCTOR"};
    auto input_str = quxlang::to_string(input);

    auto user_defined_ctor = co_await rpnx::querygraph::request< functum_user_overloads_query >(ctor_symbol);

    auto ctor_call_type = invotype{.named{{"THIS", nvalue_slot{input}}}};
    auto ctor_default_intertype = intertype{.named{{"THIS", argif{.type = nvalue_slot{input}}}}};

    // Look through destructors to find default destructor

    for (auto& ol : user_defined_ctor)
    {
        auto candidate = co_await rpnx::querygraph::request< function_ensig_init_with_query >({.ensig = ol, .params = instatype_from_invotype(ctor_call_type), .adaptations = allowed_adaptations::none});

        if (candidate)
        {
            co_return true;
        }
    }

    co_return false;
}
