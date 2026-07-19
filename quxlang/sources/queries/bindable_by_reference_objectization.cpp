// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/bindable_by_reference_objectization_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

#include <stdexcept>
#include <vector>

#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct bindable_by_reference_objectization_helpers
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
            type_symbol type;
            source_form_kind kind;
        };

        static auto allows_source_rebinding(allowed_adaptations adaptations) -> bool
        {
            switch (adaptations)
            {
            case allowed_adaptations::source_rebinding:
            case allowed_adaptations::class_conversions:
            case allowed_adaptations::destination_rebinding:
                return true;
            case allowed_adaptations::none:
                return false;
            }
            throw compiler_bug("unreachable allowed_adaptations");
        }

        static void append_source_form(std::vector< source_form >& forms, type_symbol type, source_form_kind kind)
        {
            for (source_form const& existing : forms)
            {
                if (existing.type == type)
                {
                    return;
                }
            }
            forms.push_back(source_form{.type = std::move(type), .kind = kind});
        }

        static auto enumerate_source_forms(type_symbol const& from, allowed_adaptations adaptations) -> std::vector< source_form >
        {
            std::vector< source_form > forms;
            append_source_form(forms, from, source_form_kind::exact);
            if (!allows_source_rebinding(adaptations))
            {
                return forms;
            }

            if (is_ref(from))
            {
                type_symbol const value_type = remove_ref(from);

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
    };
} // namespace quxlang::detail

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

    for (detail::bindable_by_reference_objectization_helpers::source_form const& probe :
         detail::bindable_by_reference_objectization_helpers::enumerate_source_forms(from, allowed_adaptations::source_rebinding))
    {
        if (probe.kind == detail::bindable_by_reference_objectization_helpers::source_form_kind::objectization)
        {
            continue;
        }

        if (probe.kind != detail::bindable_by_reference_objectization_helpers::source_form_kind::exact && !co_await rpnx::querygraph::request< bindable_by_reference_requalification_query >(implicitly_convertible_to_input{
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
