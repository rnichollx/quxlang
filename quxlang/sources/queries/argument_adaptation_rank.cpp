// Copyright 2023-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/argument_adaptation_rank_spec.hpp>
#include "quxlang/manipulators/typeutils.hpp"
#include "quxlang/manipulators/numeric_literal_utils.hpp"

#include <optional>
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

        throw quxlang::compiler_bug("unreachable allowed_adaptations");
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

        if (typeis< attached_type_reference >(from))
        {
            return forms;
        }

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
            if (is_temp_ref(from) && remove_ref(to) == remove_ref(from))
            {
                if (is_temp_ref(to))
                {
                    return 2;
                }

                if (is_const_ref(to))
                {
                    return 4;
                }
            }

            return 3;
        }

        return 5;
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::argument_adaptation_rank_spec > quxlang::argument_adaptation_rank_impl(argument_init_input input)
{
    auto from = input.from;
    auto const& to = input.to;

    if (from == to)
    {
        co_return 1;
    }

    if (typeis< numeric_literal_type >(from) && typeis< int_type >(to))
    {
        auto const& int_to = as< int_type >(to);
        auto const& nlt = as< numeric_literal_type >(from);
        if (!literal_fits_int(nlt.value, int_to))
        {
            co_return std::nullopt;
        }
        co_return int_to.has_sign ? 3 : 2;
    }

    if (typeis< numeric_literal_type >(from) && typeis< float_type >(to))
    {
        auto const& nlt = as< numeric_literal_type >(from);
        if (!literal_fits_float(nlt.value, as< float_type >(to)))
        {
            co_return std::nullopt;
        }
        co_return 4;
    }

    if (typeis< numeric_literal_type >(from) && typeis< readonly_constant >(to) && as< readonly_constant >(to).kind == constant_kind::numeric)
    {
        co_return 2;
    }

    if (typeis< string_literal_type >(from) && typeis< readonly_constant >(to) && as< readonly_constant >(to).kind == constant_kind::string)
    {
        co_return 2;
    }

    if (is_template(to))
    {
        for (auto const& probe : enumerate_source_forms(from, input.adaptations))
        {
            if (probe.kind != source_form_kind::exact && !co_await rpnx::querygraph::request< bindable_query >(implicitly_convertible_to_input{
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

            auto rank = template_probe_rank(from, match->type, probe.kind);
            if (rank.has_value())
            {
                co_return rank;
            }
        }
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
        co_return direct_binding_rank(from, to);
    }

    if ((co_await rpnx::querygraph::request< argument_initialize_by_class_conversion_query >(input)).has_value())
    {
        co_return 8;
    }

    co_return std::nullopt;
}
