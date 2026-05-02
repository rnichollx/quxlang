// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/argument_initialize_by_class_conversion_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

#include <stdexcept>
#include <vector>

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

} // namespace

rpnx::querygraph::coroutine< quxlang::argument_initialize_by_class_conversion_spec > quxlang::argument_initialize_by_class_conversion_impl(argument_init_input input)
{
    auto from = input.from;
    auto const& to = input.to;

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if (!allows_class_conversions(input.adaptations) || is_template(to))
    {
        co_return std::nullopt;
    }

    auto destination_value_type = to;
    if (is_ref(to))
    {
        if (!allows_destination_rebinding(input.adaptations) || !is_class_conversion_reference_target(to))
        {
            co_return std::nullopt;
        }

        destination_value_type = remove_ref(to);
    }

    if (remove_ref(from) == destination_value_type)
    {
        co_return std::nullopt;
    }

    auto constructor_functum = submember{
        .of = destination_value_type,
        .name = "CONSTRUCTOR",
    };

    for (auto const& probe : enumerate_source_forms(from, input.adaptations))
    {
        if (probe.kind != source_form_kind::exact && !co_await rpnx::querygraph::request< bindable_query >(implicitly_convertible_to_input{
                                                                    .from = from,
                                                                    .to = probe.type,
                                                                }))
        {
            continue;
        }

        auto init = initialization_reference{
            .initializee = constructor_functum,
            .parameters = instatype_from_invotype(invotype{.named = {{"OTHER", probe.type}, {"THIS", nvalue_slot{.target = destination_value_type}}}}),
            .adaptations = allowed_adaptations::source_rebinding,
        };

        if ((co_await rpnx::querygraph::request< functum_initialize_query >(init)).has_value())
        {
            co_return to;
        }
    }

    co_return std::nullopt;
}
