// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

#include <stdexcept>

namespace
{
    auto allows_source_rebinding(quxlang::allowed_adaptations adaptations) -> bool
    {
        using quxlang::allowed_adaptations;

        switch (adaptations)
        {
        case allowed_adaptations::source_rebinding:
        case allowed_adaptations::class_conversions:
        case allowed_adaptations::destination_rebinding:
            return true;
        case allowed_adaptations::none:
            return false;
        }

        throw std::logic_error("unreachable allowed_adaptations");
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::ensig_argument_initialize_spec > quxlang::ensig_argument_initialize_impl(argument_init_input input)
{
    auto from = input.from;
    auto const& to = input.to;

    if (from == to)
    {
        co_return to;
    }

    if (auto intrinsic = co_await rpnx::querygraph::request< argument_initialize_by_intrinsic_query >(argument_init_input{
                                                       .from = from,
                                                       .to = to,
                                                       .adaptations = input.adaptations,
                                                   }))
    {
        co_return intrinsic;
    }

    if (auto templated = co_await rpnx::querygraph::request< argument_initialize_by_template_query >(argument_init_input{
                                                      .from = from,
                                                      .to = to,
                                                      .adaptations = input.adaptations,
                                                  }))
    {
        co_return templated;
    }

    if (typeis< attached_type_reference >(from) || typeis< attached_type_reference >(to))
    {
        co_return std::nullopt;
    }

    if (allows_source_rebinding(input.adaptations) && co_await rpnx::querygraph::request< bindable_query >(implicitly_convertible_to_input{
                                                              .from = from,
                                                              .to = to,
                                                          }))
    {
        co_return to;
    }

    co_return co_await rpnx::querygraph::request< argument_initialize_by_class_conversion_query >(argument_init_input{
                                                        .from = from,
                                                        .to = to,
                                                        .adaptations = input.adaptations,
                                                    });
}
