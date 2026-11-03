// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/user_default_dtor_exists_spec.hpp>
#include <quxlang/data/basic_types.hpp>
#include "quxlang/keywords.hpp"
#include "quxlang/manipulators/typeutils.hpp"
#include "rpnx/unimplemented.hpp"


rpnx::querygraph::coroutine< quxlang::user_default_dtor_exists_spec > quxlang::user_default_dtor_exists_impl(type_symbol input)
{
    type_symbol const dtor_symbol = submember{.of = input, .name = "DESTRUCTOR"};
    functum_map_formal_ensigs_output_type const& overloads = co_await rpnx::querygraph::request< functum_map_user_formal_ensigs_query >(dtor_symbol);
    invotype const dtor_call_type{.named{{"THIS", make_mref(input)}}};

    for (std::pair< temploid_ensig const, std::size_t > const& overload : overloads)
    {
        std::optional< instatype > const& candidate = co_await rpnx::querygraph::request< function_ensig_init_with_query >({
            .ensig = overload.first,
            .params = instatype_from_invotype(dtor_call_type),
            .adaptations = allowed_adaptations::none,
            .type_of_this = input,
        });

        if (!candidate.has_value())
        {
            continue;
        }
        if (overload.first.enable_if.has_value())
        {
            constexpr_input const condition{
                .expr = *overload.first.enable_if,
                .context = instanciation_reference{.temploid = temploid_reference{.templexoid = dtor_symbol, .overload_id = overload.second}, .params = *candidate},
            };
            if (!(co_await rpnx::querygraph::request< constexpr_bool_query >(condition)))
            {
                continue;
            }
        }
        co_return true;
    }

    co_return false;
}
