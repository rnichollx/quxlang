// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/functum.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_select_function)
{
    if (typeis< temploid_reference >(input.initializee))
    {
        // TODO: We should identify a real match and error if this isn't a valid selection.
        // E.g. if there are type aliases, we should return the "real" type here instead of the type alias.
        // There should also be a selection error when this selection doesn't exist.
        // e.g. ::myint ALIAS I32;
        // ::foo FUNCTION(%x I32) ...
        // Would result in the following selection:
        // calle=foo#[::myint] params=(...) -> foo#[I32]

        QUX_CO_ANSWER(as< temploid_reference >(input.initializee));
    }

    auto sym_kind = co_await QUX_CO_DEP(symbol_type, (input.initializee));

    assert(sym_kind == symbol_kind::functum);

    auto overloads = co_await QUX_CO_DEP(list_functum_overloads, (input.initializee));

    std::set< temploid_reference > best_match;
    std::optional< std::int64_t > highest_priority;

    for (auto const& o : overloads)
    {
        auto candidate = co_await QUX_CO_DEP(function_ensig_initialize_with, ({.ensig = o, .params = input.parameters}));
        if (candidate)
        {
            std::size_t priority = o.priority.value_or(0);

            if (!highest_priority || priority > *highest_priority)
            {
                highest_priority = priority;
                best_match.clear();
                best_match.insert({ .templexoid = input.initializee,.which = o});
            }
            else if (priority == *highest_priority)
            {
                best_match.insert({.templexoid = input.initializee, .which = o});
            }
        }
    }

    if (best_match.size() == 0)
    {
        QUX_WHY("No matching overloads");
        QUX_CO_ANSWER(std::nullopt);
        // throw std::logic_error("No matching overloads");
    }
    else if (best_match.size() > 1)
    {
        QUX_WHY("Ambiguous overload");
        QUX_CO_ANSWER(std::nullopt);
    }

    QUX_CO_ANSWER(*best_match.begin());
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_exists_and_is_callable_with)
{
    auto ol = co_await QUX_CO_DEP(functum_initialize, (input_val));

    QUX_CO_ANSWER(ol.has_value());
}