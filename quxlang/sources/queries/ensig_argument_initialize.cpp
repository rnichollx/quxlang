// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/ensig_argument_initialize_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

#include <array>

namespace
{
    enum class source_form_kind
    {
        exact,
        temporary_materialization,
        const_rebinding,
        write_rebinding,
        objectization,
    };

    struct source_form
    {
        quxlang::type_symbol type;
        source_form_kind kind;
    };

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

    auto allows_class_conversions(quxlang::allowed_adaptations adaptations) -> bool
    {
        using quxlang::allowed_adaptations;

        switch (adaptations)
        {
        case allowed_adaptations::class_conversions:
        case allowed_adaptations::destination_rebinding:
            return true;
        case allowed_adaptations::source_rebinding:
        case allowed_adaptations::none:
            return false;
        }

        throw std::logic_error("unreachable allowed_adaptations");
    }

    auto allows_destination_rebinding(quxlang::allowed_adaptations adaptations) -> bool
    {
        return adaptations == quxlang::allowed_adaptations::destination_rebinding;
    }

    void append_source_form(std::vector< source_form >& forms, quxlang::type_symbol type, source_form_kind kind)
    {
        for (auto const& existing : forms)
        {
            if (existing.type == type)
            {
                return;
            }
        }

        forms.push_back(source_form{
            .type = std::move(type),
            .kind = kind,
        });
    }

    auto enumerate_source_forms(quxlang::type_symbol const& from, quxlang::allowed_adaptations adaptations) -> std::vector< source_form >
    {
        using namespace quxlang;

        std::vector< source_form > forms;
        append_source_form(forms, from, source_form_kind::exact);

        if (!allows_source_rebinding(adaptations))
        {
            return forms;
        }

        if (is_ref(from))
        {
            auto const value_type = remove_ref(from);

            append_source_form(forms, make_cref(value_type), source_form_kind::const_rebinding);
            append_source_form(forms, make_wref(value_type), source_form_kind::write_rebinding);

            if (!is_write_ref(from))
            {
                append_source_form(forms, value_type, source_form_kind::objectization);
            }
        }
        else
        {
            append_source_form(forms, make_tref(from), source_form_kind::temporary_materialization);
            append_source_form(forms, make_cref(from), source_form_kind::const_rebinding);
        }

        return forms;
    }

    auto is_class_conversion_reference_target(quxlang::type_symbol const& to) -> bool
    {
        return quxlang::is_temp_ref(to) || quxlang::is_const_ref(to);
    }

    auto template_probe_rank(quxlang::type_symbol const& from, quxlang::type_symbol const& adapted_type, source_form_kind kind) -> std::optional< std::size_t >
    {
        using namespace quxlang;

        if (!is_ref(from))
        {
            if (kind == source_form_kind::exact)
            {
                return 3;
            }

            if (!is_ref(adapted_type))
            {
                return std::nullopt;
            }

            if (kind == source_form_kind::temporary_materialization)
            {
                return 4;
            }

            if (kind == source_form_kind::const_rebinding)
            {
                return 6;
            }

            return std::nullopt;
        }

        if (kind == source_form_kind::exact)
        {
            if (!is_ref(adapted_type))
            {
                return std::nullopt;
            }

            if (adapted_type == from)
            {
                return 2;
            }

            return 4;
        }

        if (kind == source_form_kind::objectization)
        {
            if (is_ref(adapted_type))
            {
                return std::nullopt;
            }

            return 6;
        }

        if (is_ref(adapted_type))
        {
            return 4;
        }

        return std::nullopt;
    }

    auto direct_binding_rank(quxlang::type_symbol const& from, quxlang::type_symbol const& to) -> std::optional< std::size_t >
    {
        using namespace quxlang;

        if (!is_ref(from))
        {
            if (is_temp_ref(to) && remove_ref(to) == from)
            {
                return 2;
            }

            if (is_const_ref(to) && remove_ref(to) == from)
            {
                return 5;
            }

            return std::nullopt;
        }

        if (is_ref(to))
        {
            return 3;
        }

        return 5;
    }
} // namespace


rpnx::querygraph::coroutine< quxlang::ensig_argument_initialize_spec > quxlang::ensig_argument_initialize_impl(argument_init_input input)
{
    auto from = input.from;
    auto const& to = input.to;

    if (typeis< attached_type_reference >(from))
    {
        co_return co_await rpnx::querygraph::query_request< ensig_argument_initialize_query >(argument_init_input{
                                                                      .from = as< attached_type_reference >(from).carrying_type,
                                                                      .to = to,
                                                                      .adaptations = input.adaptations,
                                                                  });
    }

    if (from == to)
    {
        co_return to;
    }

    if (auto intrinsic = co_await rpnx::querygraph::query_request< argument_initialize_by_intrinsic_query >(argument_init_input{
                                                       .from = from,
                                                       .to = to,
                                                       .adaptations = input.adaptations,
                                                   }))
    {
        co_return intrinsic;
    }

    if (auto templated = co_await rpnx::querygraph::query_request< argument_initialize_by_template_query >(argument_init_input{
                                                      .from = from,
                                                      .to = to,
                                                      .adaptations = input.adaptations,
                                                  }))
    {
        co_return templated;
    }

    if (allows_source_rebinding(input.adaptations) && co_await rpnx::querygraph::query_request< bindable_query >(implicitly_convertible_to_input{
                                                              .from = from,
                                                              .to = to,
                                                          }))
    {
        co_return to;
    }

    co_return co_await rpnx::querygraph::query_request< argument_initialize_by_class_conversion_query >(argument_init_input{
                                                        .from = from,
                                                        .to = to,
                                                        .adaptations = input.adaptations,
                                                    });
}
