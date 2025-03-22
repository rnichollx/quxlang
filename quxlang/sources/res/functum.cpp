// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/compiler.hpp>
#include <quxlang/res/functum.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_select_function)
{


    auto input_str = to_string(input_val);

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

    if (sym_kind != symbol_kind::functum)
    {
        co_return std::nullopt;
    }

    auto overloads = co_await QUX_CO_DEP(functum_overloads, (input.initializee));

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
                best_match.insert({.templexoid = input.initializee, .which = o});
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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_builtin_overloads)
{
    auto const& primitive_overloads = co_await QUX_CO_DEP(functum_primitive_overloads, (input));

    std::set< temploid_ensig > results;

    for (auto const& info : primitive_overloads)
    {
        results.insert(info.overload);
    }

    // TODO: Add other builtin non-primitive overloads here.

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_map_user_formal_ensigs)
{
    auto const& decls = co_await QUX_CO_DEP(functum_list_user_ensig_declarations, (input));

    std::string input_name = quxlang::to_string(input);

    std::map< temploid_ensig, std::size_t > output;

    bool is_member_functum = typeis< submember >(input);
    std::optional<type_symbol> class_type;
    bool is_ctor = false;
    bool is_dtor = false;
    if (is_member_functum)
    {
        submember const& m = as< submember >(input);
        class_type = m.of;
        if (m.name == "CONSTRUCTOR")
        {
            is_ctor = true;
        }
        else if (m.name == "DESTRUCTOR")
        {
            is_dtor = true;
        }
    }

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const &decl = decls.at(i);
        temploid_ensig formal_ensig;
        formal_ensig.priority = decl.priority;
        for (auto const& param : decl.interface.named)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.second.type;
            declared_type_with_context.context = input;
            auto const& formal_type_opt = co_await QUX_CO_DEP(lookup, (declared_type_with_context));
            if (!formal_type_opt.has_value())
            {
                throw std::logic_error("Type not found");
            }
            formal_ensig.interface.named[param.first] = argif{.type = formal_type_opt.value(), .is_defaulted = param.second.is_defaulted};
        }
        for (auto const& param : decl.interface.positional)
        {
            contextual_type_reference declared_type_with_context;
            declared_type_with_context.type = param.type;
            declared_type_with_context.context = input;
            auto const& formal_type_opt = co_await QUX_CO_DEP(lookup, (declared_type_with_context));
            if (!formal_type_opt.has_value())
            {
                throw std::logic_error("Type not found");
            }
            formal_ensig.interface.positional.push_back(argif{.type = formal_type_opt.value(), .is_defaulted = param.is_defaulted});
        }

        if (is_member_functum && !formal_ensig.interface.named.contains("THIS"))
        {
            argif this_argif;

            if (is_ctor)
            {
                this_argif.type = nvalue_slot{.target = class_type.value()};
            }
            else if (is_dtor)
            {
                this_argif.type = dvalue_slot{.target = class_type.value()};
            }
            else
            {
                this_argif.type = pointer_type{.target = class_type.value(), .ptr_class = pointer_class::ref, .qual = qualifier::auto_};
            }

            formal_ensig.interface.named["THIS"] = this_argif;
        }

        if (output.contains(formal_ensig))
        {
            throw std::logic_error("Duplicate overload");
        }

        output.insert({formal_ensig, i});
    }

    co_return output;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_user_overloads)
{
    auto const& map = co_await QUX_CO_DEP(functum_map_user_formal_ensigs, (input));

    std::set< temploid_ensig > results;

    for (auto const& [ensig, index] : map)
    {
        results.insert(ensig);
    }

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_list_user_ensig_declarations)
{
    auto const& decls = co_await QUX_CO_DEP(functum_list_user_overload_declarations, (input));

    std::vector< temploid_ensig > output;

    for (std::size_t i = 0; i < decls.size(); i++)
    {
        auto const& head = decls.at(i).header;

        temploid_ensig ensig;
        ensig.priority = head.priority;

        for (std::size_t y = 0; y < head.call_parameters.size(); y++)
        {
            auto const& param = head.call_parameters.at(y);

            argif arg;
            if (param.default_expr.has_value())
            {
                arg.is_defaulted = true;
            }

            arg.type = param.type;

            if (param.api_name.has_value())
            {
                if (ensig.interface.named.contains(param.api_name.value()))
                {
                    throw std::logic_error("Duplicate parameter name");
                    //
                }

                ensig.interface.named[param.api_name.value()] = arg;
            }
            else
            {
                ensig.interface.positional.push_back(arg);
            }
        }

        output.push_back(ensig);
    }

    co_return output;
}
