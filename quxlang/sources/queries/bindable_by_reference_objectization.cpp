// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/bindable_by_reference_objectization_spec.hpp>
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

} // namespace

rpnx::querygraph::coroutine< quxlang::bindable_by_reference_objectization_spec > quxlang::bindable_by_reference_objectization_impl(implicitly_convertible_to_input input)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || is_ref(to) || !is_ref(from))
    {
        co_return false;
    }

    auto constructor_functum = submember{
        .of = to,
        .name = "CONSTRUCTOR",
    };

    for (auto const& probe : enumerate_source_forms(from, allowed_adaptations::source_rebinding))
    {
        if (probe.kind == source_form_kind::objectization)
        {
            continue;
        }

        if (probe.kind != source_form_kind::exact && !co_await rpnx::querygraph::request< bindable_by_reference_requalification_query >(implicitly_convertible_to_input{
                                                               .from = from,
                                                               .to = probe.type,
                                                           }))
        {
            continue;
        }

        auto init = initialization_reference{
            .initializee = constructor_functum,
            .parameters = instatype_from_invotype(invotype{.named = {{"OTHER", probe.type}, {"THIS", nvalue_slot{.target = to}}}}),
            .adaptations = allowed_adaptations::none,
        };

        if ((co_await rpnx::querygraph::request< functum_initialize_query >(init)).has_value())
        {
            co_return true;
        }
    }

    co_return false;
}
