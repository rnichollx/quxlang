// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/res/implicitly_convertible_to.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/manipulators/typeutils.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(implicitly_convertible_to)
{
    type_symbol from = input.from;
    type_symbol to = input.to;

    std::string from_str = quxlang::to_string(from);
    std::string to_str = quxlang::to_string(to);

    if (from == to)
    {
        std::cout << "Convertible: " << from_str << " to " << to_str << " (exact match)" << std::endl;
        co_return true;
    }

    if (remove_ref(to) == remove_ref(from))
    {

        if (is_const_ref(to) && !is_write_ref(from))
        {
            // All value/reference types can be implicitly cast to CONST&
            // except OUT& references
            std::cout << "Convertible: " << from_str << " to " << to_str << " (ref cast)" << std::endl;
            co_return true;
        }
        else if (!is_ref(from) && (is_temp_ref(to) || is_write_ref(to)))
        {
            // TODO: Allow ivalue pseudo-type here.

            // We can convert a temporary value into a TEMP& reference
            // implicitly.
            co_return true;
        }
        else if (is_mut_ref(from) && is_write_ref(to))
        {
            // Mutable references can be implicitly cast to output references.
            co_return true;
        }
        else if (!is_ref(to) && !is_write_ref(from))
        {
            co_return true;
        }
    }

    std::string to_type_str = to_string(to);

    std::string from_type_str = to_string(from);

    if (typeis< int_type >(to) && typeis< numeric_literal_reference >(from))
    {
        co_return true;
    }

    if (typeis< attached_type_reference >(from))
    {
        auto from_unbound = as< attached_type_reference >(from).carrying_type;
        co_return co_await QUX_CO_DEP(implicitly_convertible_to, (implicitly_convertible_to_query{.from = from_unbound, .to = to}));
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_adaptation_rank)
{
    auto from = input.from;
    auto to = input.to;
    auto init_kind = input.init_kind;

    assert(init_kind != parameter_init_kind::none);

    if (typeis< attached_type_reference >(from))
    {
        from = as< attached_type_reference >(from).carrying_type;
    }

    if (from == to)
    {
        co_return 1;
    }

    auto nonbinding_rank = [&]() -> std::size_t
    {
        return is_ref(from) ? 11 : 8;
    };

    if (!is_ref(from))
    {
        if (is_temp_ref(to) && remove_ref(to) == from)
        {
            co_return 2;
        }

        if (is_template(to) && match_template(to, from).has_value())
        {
            co_return 3;
        }

        auto temp_match = make_tref(from);
        if (is_template(to) && match_template(to, temp_match).has_value())
        {
            co_return 4;
        }

        if (is_const_ref(to) && remove_ref(to) == from)
        {
            co_return 5;
        }

        auto const_match = make_cref(from);
        if (is_template(to) && match_template(to, const_match).has_value())
        {
            co_return 6;
        }

        auto builtin_binding = co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (builtin_binding)
        {
            co_return 7;
        }
    }
    else
    {
        if (is_template(to) && match_template(to, from).has_value())
        {
            co_return 2;
        }

        auto ref_requal = co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (ref_requal)
        {
            co_return 3;
        }

        auto unbound_from = remove_ref(from);

        if (!is_ref(to) && remove_ref(to) == unbound_from)
        {
            auto constructor_functum = submember{.of = to, .name = "CONSTRUCTOR"};
            auto exact_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::bind_only}));

            if (exact_init.has_value())
            {
                if (is_temp_ref(from))
                {
                    co_return 6;
                }
                if (is_const_ref(from))
                {
                    co_return 8;
                }
                co_return 4;
            }

            auto target_const_ref = make_cref(to);
            auto const_requalifiable = co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = target_const_ref}));
            if (const_requalifiable)
            {
                auto const_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", target_const_ref}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::bind_only}));

                if (const_init.has_value())
                {
                    // Direct CONST& objectization is handled by the exact-source probe above.
                    if (is_const_ref(from))
                    {
                        co_return 8;
                    }
                    co_return 9;
                }
            }
        }

        if (is_template(to))
        {
            auto objectized_match = match_template(to, unbound_from);
            if (objectized_match.has_value() && !is_ref(objectized_match->type))
            {
                auto matched_type = objectized_match->type;
                auto constructor_functum = submember{.of = matched_type, .name = "CONSTRUCTOR"};
                auto exact_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}, {"THIS", nvalue_slot{.target = matched_type}}}}, .init_kind = parameter_init_kind::bind_only}));

                if (exact_init.has_value())
                {
                    if (is_temp_ref(from))
                    {
                        co_return 7;
                    }
                    if (is_const_ref(from))
                    {
                        co_return 10;
                    }
                    co_return 5;
                }

                auto target_const_ref = make_cref(matched_type);
                auto const_requalifiable = co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = target_const_ref}));
                if (const_requalifiable)
                {
                    auto const_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", target_const_ref}, {"THIS", nvalue_slot{.target = matched_type}}}}, .init_kind = parameter_init_kind::bind_only}));

                    if (const_init.has_value())
                    {
                        // Direct CONST& objectization is handled by the exact-source probe above.
                        if (is_const_ref(from))
                        {
                            co_return 10;
                        }
                        co_return 11;
                    }
                }
            }
        }

        auto builtin_binding = co_await QUX_CO_DEP(bindable, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (builtin_binding)
        {
            co_return 10;
        }
    }

    auto adapted = co_await QUX_CO_DEP(ensig_argument_initialize, (argument_init_query{.from = from, .to = to, .init_kind = init_kind}));
    if (adapted.has_value())
    {
        co_return nonbinding_rank();
    }

    co_return std::nullopt;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(argument_adaptation_is_better_fit)
{
    auto better_rank = co_await QUX_CO_DEP(argument_adaptation_rank, (argument_init_query{
                                                                         .from = input.from,
                                                                         .to = input.better_to,
                                                                         .init_kind = input.init_kind,
                                                                     }));

    if (!better_rank.has_value())
    {
        co_return false;
    }

    auto worse_rank = co_await QUX_CO_DEP(argument_adaptation_rank, (argument_init_query{
                                                                        .from = input.from,
                                                                        .to = input.worse_to,
                                                                        .init_kind = input.init_kind,
                                                                    }));

    if (!worse_rank.has_value())
    {
        co_return false;
    }

    co_return *better_rank < *worse_rank;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(ensig_argument_initialize)
{
    type_symbol from = input.from;
    type_symbol to = input.to;
    parameter_init_kind init_kind = input.init_kind;

    std::vector< std::byte > init_kind_bytes;
    rpnx::serial4::json_serialize_iter(init_kind, std::back_inserter(init_kind_bytes));
    std::string init_kind_str;
    for (auto b : init_kind_bytes)
    {
        init_kind_str += static_cast< char >(b);
    }

    std::cout << "Ensig Argument Initialize: from " << quxlang::to_string(from) << " to " << quxlang::to_string(to) << " (init kind: " << init_kind_str << ")" << std::endl;

    std::string from_str = quxlang::to_string(from);
    std::string to_str = quxlang::to_string(to);
    if (to_str.contains("CONSTRUCTOR"))
    {
        auto deps = this->dependents();
        for (auto const& dep : deps)
        {
            std::cout << " Dependent: " << dep->question() << std::endl;
        }
        int breakpoint = 0;
    }

    assert(input.init_kind != parameter_init_kind::none);

    if (from == to)
    {
        // Identity initialization
        std::cout << "   Ensig Arg OK(" << from_str << " to " << to_str << ") (exact match)" << std::endl;
        co_return to;
    }

    if (is_template(to))
    {
        auto match = match_template(to, from);
        if (match.has_value())
        {
            std::cout << "   Convertible: " << from_str << " to " << to_str << std::endl;
            co_return match.value().type;
        }
    }

    // if a type can be bound by reference objectization, then it's implicitly convertible.
    // We skip this check if we're already in reference objectization,
    // otherwise it would produce infinite recursion.
    if (init_kind != parameter_init_kind::bind_only)
    {
        std::cout << "  Checking argument conversion: " << from_str << " to " << to_str << std::endl;
        auto bindable_by_ref_objectization = co_await QUX_CO_DEP(bindable_by_reference_objectization, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (bindable_by_ref_objectization)
        {
            std::cout << "Found bindable by reference objectization: " << from_str << " to " << to_str << std::endl;
            co_return to;
        }
        else
        {
            std::cout << "Not bindable by reference objectization: " << from_str << " to " << to_str << std::endl;
        }
    }
    else
    {
        std::cout << "Not checking argument conversion during reference objectization: " << from_str << " to " << to_str << std::endl;
    }

    // even during reference objectization, we can rebind reference qualifiers
    auto bindable_by_ref_qual = co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = to}));
    if (bindable_by_ref_qual)
    {
        // TODO: Handle templates like INPUT& (Maybe ?)
        std::cout << "Found bindable by reference requalification: " << from_str << " to " << to_str << std::endl;
        co_return to;
    }
    else
    {
        std::cout << "Not bindable by reference requalification: " << from_str << " to " << to_str << std::endl;
    }

    auto bindable_by_temp_materialization = co_await QUX_CO_DEP(bindable_by_temporary_materialization, (implicitly_convertible_to_query{.from = from, .to = to}));
    if (bindable_by_temp_materialization)
    {
        std::cout << "Found bindable by temporary materialization: " << from_str << " to " << to_str << std::endl;
        co_return to;
    }
    else
    {
        std::cout << "Not bindable by temporary materialization: " << from_str << " to " << to_str << std::endl;
    }

    if (is_template(to) && !is_ref(from))
    {
        auto temp_materialized_match = match_template(to, make_tref(from));
        if (temp_materialized_match.has_value())
        {
            std::cout << "Found bindable by temporary materialization via template: " << from_str << " to " << to_str << std::endl;
            co_return temp_materialized_match->type;
        }

        auto const_materialized_match = match_template(to, make_cref(from));
        if (const_materialized_match.has_value())
        {
            std::cout << "Found bindable by const materialization via template: " << from_str << " to " << to_str << std::endl;
            co_return const_materialized_match->type;
        }
    }

    if (is_template(to) && is_ref(from))
    {
        auto objectized_match = match_template(to, remove_ref(from));
        if (objectized_match.has_value() && !is_ref(objectized_match->type))
        {
            auto bindable_by_objectization = co_await QUX_CO_DEP(bindable_by_reference_objectization, (implicitly_convertible_to_query{.from = from, .to = objectized_match->type}));
            if (bindable_by_objectization)
            {
                std::cout << "Found bindable by reference objectization via template: " << from_str << " to " << to_str << std::endl;
                co_return objectized_match->type;
            }
        }
    }

    // Otherwise, unless this is a conversion, we can test if the type is convertible by constructor call
    if (init_kind != parameter_init_kind::implicit_conversion && init_kind != parameter_init_kind::bind_only)
    {

        auto convertible_by_call = co_await QUX_CO_DEP(convertible_by_call, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (convertible_by_call.has_value())
        {
            std::cout << "Found convertible by call: " << from_str << " to " << to_str << std::endl;
            co_return convertible_by_call.value();
        }
        else
        {
            std::cout << "Not convertible by call: " << from_str << " to " << to_str << std::endl;
        }
    }
    else
    {
        std::cout << "Not checking convertible by call during conversion/reference objectization: " << from_str << " to " << to_str << std::endl;
    }

    if (typeis< attached_type_reference >(from))
    {
        auto from_disattached = as< attached_type_reference >(from).carrying_type;
        co_return co_await QUX_CO_DEP(ensig_argument_initialize, (argument_init_query{.from = from_disattached, .to = to, .init_kind = init_kind}));
    }

    co_return std::nullopt;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable)
{
    type_symbol from = input.from;
    type_symbol to = input.to;

    std::string from_str = quxlang::to_string(from);
    std::string to_str = quxlang::to_string(to);

    assert((!typeis< attached_type_reference >(from) && !typeis< attached_type_reference >(to) && !typeis< attached_type_reference >(remove_ref(from)) && !typeis< attached_type_reference >(remove_ref(to))) && "Bindable resolver does not support symbol-attached types.");

    // Types are bindable to themselves.
    if (from == to)
    {
        std::cout << "Convertible: " << from_str << " to " << to_str << " (exact match)" << std::endl;
        co_return true;
    }

    // Bindable types need to be the same underlying type.
    if (remove_ref(to) != remove_ref(from))
    {
        co_return false;
    }

    // (T & -> T)
    // If .CONSTRUCTOR can accept T CONST&,

    if (!is_ref(from) && is_ref(to))
    {
        // This is a temporary materialization binding.
        co_return co_await QUX_CO_DEP(bindable_by_temporary_materialization, (implicitly_convertible_to_query{.from = from, .to = to}));
    }

    if (is_ref(from) && is_ref(to))
    {
        // Both are reference types.
        co_return co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = to}));
    }

    if (is_ref(from) && !is_ref(to))
    {
        // Reference objectization binding
        co_return co_await QUX_CO_DEP(bindable_by_reference_objectization, (implicitly_convertible_to_query{.from = from, .to = to}));
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_reference_requalification)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || !is_ref(to) || !is_ref(from))
    {
        co_return false;
    }

    ptrref_type const& from_ref = as< ptrref_type >(from);
    ptrref_type const& to_ref = as< ptrref_type >(to);

    auto qual_match = qualifier_template_match(to_ref.qual, from_ref.qual);

    co_return qual_match.has_value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_temporary_materialization)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || !is_ref(to) || is_ref(from))
    {
        co_return false;
    }

    ptrref_type const& to_ref = as< ptrref_type >(to);

    auto okay = qualifier_template_match(to_ref.qual, qualifier::temp);

    co_return okay.has_value();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_reference_objectization)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || is_ref(to) || !is_ref(from))
    {
        std::cout << " Not bindable by reference objectization (type or ref mismatch): " << to_string(from) << " to " << to_string(to) << std::endl;
        co_return false;
    }

    auto constructor_functum = submember{.of = to, .name = "CONSTRUCTOR"};
    auto target_const_ref = make_cref(to);

    std::cout << " Checking bindable by reference objectization: " << to_string(from) << " to " << to_string(to) << " via " << to_string(constructor_functum) << std::endl;

    auto exact_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::bind_only}));
    bool exact_bindable = exact_init.has_value();

    bool const_bindable = false;
    auto const_requalifiable = co_await QUX_CO_DEP(bindable_by_reference_requalification, (implicitly_convertible_to_query{.from = from, .to = target_const_ref}));
    if (!exact_bindable && const_requalifiable)
    {
        auto const_init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", target_const_ref}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::bind_only}));
        const_bindable = const_init.has_value();
    }

    if (!exact_bindable && !const_bindable)
    {
        std::cout << "  Not bindable by reference objectization (no exact/CONST& path): " << to_string(from) << " to " << to_string(to) << std::endl;
    }
    else
    {
        std::cout << "  X Bindable by reference objectization: " << to_string(from) << " to " << to_string(to) << std::endl;
    }

    co_return exact_bindable || const_bindable;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(convertible_by_call)
{
    auto const from = input.from;
    auto const to = input.to;

    std::string to_str = to_string(to);

    if (to_str.contains("CONSTRUCTOR"))
    {
        for (auto dep : this->dependents())
        {
            std::cout << " Dependent: " << dep->question() << std::endl;
        }

        int debugbreakpoint = 0;
    }

    auto constructor_functum = submember{.of = to, .name = "CONSTRUCTOR"};

    std::string constructor_functum_name = to_string(constructor_functum);

    std::cout << "Ctor name: " << constructor_functum_name << std::endl;

    auto call_param = initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::implicit_conversion};

    auto init = co_await QUX_CO_DEP(functum_initialize, (call_param));

    if (init.has_value())
    {
        co_return to;
    }
    else
    {
        co_return std::nullopt;
    }
}
