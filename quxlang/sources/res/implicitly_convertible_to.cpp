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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(ensig_argument_initialize)
{
    type_symbol from = input.from;
    type_symbol to = input.to;
    parameter_init_kind init_kind = input.init_kind;

    std::vector<std::byte> init_kind_bytes;
    rpnx::json_serialize_iter(init_kind, std::back_inserter(init_kind_bytes));
    std::string init_kind_str;
    for (auto b : init_kind_bytes)
    {
        init_kind_str += static_cast<char>(b);
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

    if (is_template(to))
    {
        auto match = match_template(to, from);
        if (match.has_value())
        {
            co_return match.value().type;
        }
        else
        {
            co_return std::nullopt;
        }
    }

    if (from == to)
    {
        // Identity initialization
        std::cout << "   Ensig Arg OK(" << from_str << " to " << to_str << ") (exact match)" << std::endl;
        co_return to;
    }

    // if a type can be bound by argument conversion, then it's implicitly convertible.
    // We skip this check if we're already in argument construction,
    // otherwise it would produce infinite recursion.
    if (init_kind != parameter_init_kind::argument_construction)
    {
        std::cout << "  Checking argument conversion: " << from_str << " to " << to_str << std::endl;
        auto bindable_by_arg_construction = co_await QUX_CO_DEP(bindable_by_argument_construction, (implicitly_convertible_to_query{.from = from, .to = to}));
        if (bindable_by_arg_construction)
        {
            std::cout << "Found bindable by argument construction: " << from_str << " to " << to_str << std::endl;
            co_return to;
        }
        else
        {
            std::cout << "Not bindable by argument construction: " << from_str << " to " << to_str << std::endl;
        }
    }
    else
    {
        std::cout << "Not checking argument conversion during argument construction: " << from_str << " to " << to_str << std::endl;
    }

    // even during argument construction, we can rebind reference qualifiers
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

    // Otherwise, unless this is a conversion, we can test if the type is convertible by constructor call
    if (init_kind != parameter_init_kind::conversion && init_kind != parameter_init_kind::argument_construction)
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
        std::cout << "Not checking convertible by call during conversion/argument construction: " << from_str << " to " << to_str << std::endl;
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
        // Arugument construction binding
        co_return co_await QUX_CO_DEP(bindable_by_argument_construction, (implicitly_convertible_to_query{.from = from, .to = to}));
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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(bindable_by_argument_construction)
{
    auto const& from = input.from;
    auto const& to = input.to;

    if (remove_ref(to) != remove_ref(from) || is_ref(to) || !is_ref(from))
    {
        co_return false;
    }

    auto constructor_functum = submember{.of = to, .name = "CONSTRUCTOR"};

    auto init = co_await QUX_CO_DEP(functum_initialize, (initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}}}, .init_kind = parameter_init_kind::argument_construction}));

    co_return init.has_value();
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

        std::terminate();
    }

    auto constructor_functum = submember{.of = to, .name = "CONSTRUCTOR"};

    std::string constructor_functum_name = to_string(constructor_functum);

    std::cout << "Ctor name: " << constructor_functum_name << std::endl;

    auto call_param = initialization_reference{.initializee = constructor_functum, .parameters = {.named = {{"OTHER", from}, {"THIS", nvalue_slot{.target = to}}}}, .init_kind = parameter_init_kind::conversion};

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