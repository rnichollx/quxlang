// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/implicitly_convertible_to.hpp"

#include "quxlang/compiler.hpp"
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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(implicitly_convertible_to)
{
    auto adapted = co_await QUX_CO_DEP(ensig_argument_initialize, (argument_init_query{
                                                                      .from = input.from,
                                                                      .to = input.to,
                                                                      .adaptations = allowed_adaptations::destination_rebinding,
                                                                  }));
    co_return adapted.has_value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_initialize_by_intrinsic)
{
    auto from = input.from;

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if (typeis< int_type >(input.to) && typeis< numeric_literal_reference >(from))
    {
        co_return input.to;
    }

    co_return std::nullopt;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_initialize_by_template)
{
    auto from = input.from;
    auto const& to = input.to;

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if (!is_template(to))
    {
        co_return std::nullopt;
    }

    for (auto const& probe : enumerate_source_forms(from, input.adaptations))
    {
        if (probe.kind != source_form_kind::exact && !co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{
                                                                    .from = from,
                                                                    .to = probe.type,
                                                                })))
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
            if (probe.kind == source_form_kind::exact)
            {
                co_return match->type;
            }

            if (is_ref(match->type))
            {
                co_return match->type;
            }

            continue;
        }

        if (probe.kind == source_form_kind::objectization)
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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_initialize_by_class_conversion)
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
        if (probe.kind != source_form_kind::exact && !co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{
                                                                    .from = from,
                                                                    .to = probe.type,
                                                                })))
        {
            continue;
        }

        auto init = initialization_reference{
            .initializee = constructor_functum,
            .parameters = {.named = {{"OTHER", probe.type}, {"THIS", nvalue_slot{.target = destination_value_type}}}},
            .adaptations = allowed_adaptations::source_rebinding,
        };

        if ((co_await QUX_CO_DEP(functum_initialize, (init))).has_value())
        {
            co_return to;
        }
    }

    co_return std::nullopt;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_adaptation_rank)
{
    auto from = input.from;
    auto const& to = input.to;

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if (from == to)
    {
        co_return 1;
    }

    if (is_template(to))
    {
        for (auto const& probe : enumerate_source_forms(from, input.adaptations))
        {
            if (probe.kind != source_form_kind::exact && !co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{
                                                                        .from = from,
                                                                        .to = probe.type,
                                                                    })))
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

    if (allows_source_rebinding(input.adaptations) && co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{
                                                              .from = from,
                                                              .to = to,
                                                          })))
    {
        co_return direct_binding_rank(from, to);
    }

    if ((co_await QUX_CO_DEP(argument_initialize_by_class_conversion, (input))).has_value())
    {
        co_return 8;
    }

    co_return std::nullopt;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_adaptation_is_better_fit)
{
    auto better_rank = co_await QUX_CO_DEP(argument_adaptation_rank, (argument_init_query{
                                                                         .from = input.from,
                                                                         .to = input.better_to,
                                                                         .adaptations = input.adaptations,
                                                                     }));
    if (!better_rank.has_value())
    {
        co_return false;
    }

    auto worse_rank = co_await QUX_CO_DEP(argument_adaptation_rank, (argument_init_query{
                                                                        .from = input.from,
                                                                        .to = input.worse_to,
                                                                        .adaptations = input.adaptations,
                                                                    }));
    if (!worse_rank.has_value())
    {
        co_return false;
    }

    co_return *better_rank < *worse_rank;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(ensig_argument_initialize)
{
    auto from = input.from;
    auto const& to = input.to;

    if (typeis< attached_type_reference >(from))
    {
        co_return co_await QUX_CO_DEP(ensig_argument_initialize, (argument_init_query{
                                                                      .from = as< attached_type_reference >(from).carrying_type,
                                                                      .to = to,
                                                                      .adaptations = input.adaptations,
                                                                  }));
    }

    if (from == to)
    {
        co_return to;
    }

    if (auto intrinsic = co_await QUX_CO_DEP(argument_initialize_by_intrinsic, (argument_init_query{
                                                       .from = from,
                                                       .to = to,
                                                       .adaptations = input.adaptations,
                                                   })))
    {
        co_return intrinsic;
    }

    if (auto templated = co_await QUX_CO_DEP(argument_initialize_by_template, (argument_init_query{
                                                      .from = from,
                                                      .to = to,
                                                      .adaptations = input.adaptations,
                                                  })))
    {
        co_return templated;
    }

    if (allows_source_rebinding(input.adaptations) && co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{
                                                              .from = from,
                                                              .to = to,
                                                          })))
    {
        co_return to;
    }

    co_return co_await QUX_CO_DEP(argument_initialize_by_class_conversion, (argument_init_query{
                                                        .from = from,
                                                        .to = to,
                                                        .adaptations = input.adaptations,
                                                    }));
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable)
{
    auto const& from = input.from;
    auto const& to = input.to;

    assert((!typeis< attached_type_reference >(from) && !typeis< attached_type_reference >(to) && !typeis< attached_type_reference >(remove_ref(from)) && !typeis< attached_type_reference >(remove_ref(to))) && "Bindable resolver does not support symbol-attached types.");

    if (from == to)
    {
        co_return true;
    }

    if (remove_ref(to) != remove_ref(from))
    {
        co_return false;
    }

    if (!is_ref(from) && is_ref(to))
    {
        co_return co_await QUX_CO_DEP(bindable_by_temporary_materialization, (input));
    }

    if (is_ref(from) && is_ref(to))
    {
        co_return co_await QUX_CO_DEP(bindable_by_reference_requalification, (input));
    }

    if (is_ref(from) && !is_ref(to))
    {
        co_return co_await QUX_CO_DEP(bindable_by_reference_objectization, (input));
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(convertible_by_call)
{
    co_return co_await QUX_CO_DEP(argument_initialize_by_class_conversion, (argument_init_query{
                                                        .from = input.from,
                                                        .to = input.to,
                                                        .adaptations = allowed_adaptations::destination_rebinding,
                                                    }));
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_reference_requalification)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || !is_ref(to) || !is_ref(from))
    {
        co_return false;
    }

    auto const& from_ref = as< ptrref_type >(from);
    auto const& to_ref = as< ptrref_type >(to);

    co_return qualifier_template_match(to_ref.qual, from_ref.qual).has_value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_temporary_materialization)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || !is_ref(to) || is_ref(from))
    {
        co_return false;
    }

    auto const& to_ref = as< ptrref_type >(to);
    co_return qualifier_template_match(to_ref.qual, qualifier::temp).has_value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_reference_objectization)
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

        if (probe.kind != source_form_kind::exact && !co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{
                                                               .from = from,
                                                               .to = probe.type,
                                                           })))
        {
            continue;
        }

        auto init = initialization_reference{
            .initializee = constructor_functum,
            .parameters = {.named = {{"OTHER", probe.type}, {"THIS", nvalue_slot{.target = to}}}},
            .adaptations = allowed_adaptations::none,
        };

        if ((co_await QUX_CO_DEP(functum_initialize, (init))).has_value())
        {
            co_return true;
        }
    }

    co_return false;
}
