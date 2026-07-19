// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/argument_initialize_by_template_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"

#include <stdexcept>
#include <vector>

#include "query_helpers.hpp"

namespace quxlang::detail
{
    struct argument_initialize_by_template_helpers
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

            if (typeis< attached_type_reference >(from) || !allows_source_rebinding(adaptations))
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

rpnx::querygraph::coroutine< quxlang::argument_initialize_by_template_spec > quxlang::argument_initialize_by_template_impl(argument_init_input input)
{
    auto from = input.from;
    auto const& to = input.to;

    if (!is_template(to))
    {
        co_return std::nullopt;
    }

    for (detail::argument_initialize_by_template_helpers::source_form const& probe : detail::argument_initialize_by_template_helpers::enumerate_source_forms(from, input.adaptations))
    {
        if (probe.kind != detail::argument_initialize_by_template_helpers::source_form_kind::exact && !co_await rpnx::querygraph::request< bindable_query >(implicitly_convertible_to_input{
                                                                    .from = from,
                                                                    .to = probe.type,
                                                                }))
        {
            continue;
        }

        auto match = match_template(to, probe.type);
        if (!match.has_value())
        {
            continue;
        }

        if (!is_ref(from))
        {
            if (probe.kind == detail::argument_initialize_by_template_helpers::source_form_kind::exact)
            {
                co_return match->type;
            }

            if (is_ref(match->type))
            {
                co_return match->type;
            }

            continue;
        }

        if (probe.kind == detail::argument_initialize_by_template_helpers::source_form_kind::objectization)
        {
            if (!is_ref(match->type))
            {
                co_return match->type;
            }

            continue;
        }

        if (is_ref(match->type))
        {
            co_return match->type;
        }
    }

    co_return std::nullopt;
}
