// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"

#include "quxlang/res/overloads.hpp"

#include "quxlang/operators.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_overload_declarations)
{
    std::string name = to_string(input_val);

    auto const& func_addr = input_val;

    std::vector< ast2_function_declaration > result;

    QUX_CO_GETDEP(maybe_functum_ast, symboid, (input_val));

    if (!typeis< functum >(maybe_functum_ast))
    {
        // TODO: Redirect to constructor?
        co_return {};
    }

    auto const& functum_v = as< functum >(maybe_functum_ast);

    for (auto const& func : functum_v.functions)
    {
        result.push_back(func);
    }

    QUX_CO_ANSWER(result);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_formal_paratypes)
{
    std::string name = to_string(input_val);

    auto decls = co_await QUX_CO_DEP(list_user_functum_overload_declarations, (input));

    std::vector< paratype > result;


    bool is_member_func = typeis< submember >(input);
    bool is_constructor = is_member_func && input.get_as< submember >().name == "CONSTRUCTOR";
    bool is_destructor = is_member_func && input.get_as< submember >().name == "DESTRUCTOR";



    auto thistype_type = qualified_parent(input).value_or(void_type{});

    for (auto const& decl : decls)
    {
        paratype p;

        for (auto const& param : decl.header.call_parameters)
        {
            contextual_type_reference ctx_type{.context = input, .type = param.type};

            auto type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (ctx_type));

            parameter_type formal_parameter;

            formal_parameter.type = type;

            // TODO: do all symbol lookups in this context
            formal_parameter.default_value = param.default_expr;

            if (param.api_name)
            {
                p.named[param.api_name.value()] = formal_parameter;
            }
            else
            {
                p.positional.push_back(formal_parameter);
            }
        }


        if (is_member_func && !p.named.contains("THIS"))
        {
            if (!is_constructor && !is_destructor)
            {
                parameter_type default_this{};
                // By default, AUTO& THISTYPE
                default_this.type = auto_reference{.target = thistype_type};
                p.named["THIS"] = default_this;
            }
            else if (is_constructor)
            {
                parameter_type default_this{};
                default_this.type = nvalue_slot{.target = thistype_type};

                p.named["THIS"] = default_this;
            }
            else
            {
                parameter_type default_this{};
                default_this.type = dvalue_slot{.target = thistype_type};
                p.named["THIS"] = default_this;
            }
        }

        result.push_back(p);
    }

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_user_functum_overloads)
{

    std::vector< ast2_function_declaration > user_defined = co_await QUX_CO_DEP(list_user_functum_overload_declarations, (input));
    std::vector<paratype> paratypes = co_await QUX_CO_DEP(list_user_functum_formal_paratypes, (input));

    std::vector< temploid_formal_paratype > results;

    for (std::size_t i = 0; i < user_defined.size(); ++i)
    {
        paratype p = paratypes.at(i);
        ast2_function_declaration const& decl = user_defined.at(i);

        temploid_formal_paratype ol;
        ol.builtin = false;
        ol.priority = decl.header.priority;

        for (auto const& [name, value] : p.named)
        {
            ol.call_parameters.named[name] = value.type;
        }

        for (std::size_t i = 0; i < p.positional.size(); ++i)
        {
            ol.call_parameters.positional.push_back(p.positional[i].type);
        }

        results.push_back(ol);
    }

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_functum_overloads)
{
    QUX_CO_GETDEP(builtins, list_builtin_functum_overloads, (input));
    auto user_defined_overloads = co_await QUX_CO_DEP(list_user_functum_overloads, (input));


    std::string name = to_string(input);
    std::set< temploid_formal_paratype > all_overloads;
    for (auto const& o : builtins)
    {
        assert(o.overload.builtin == true);
        all_overloads.insert(o.overload);
    }

    for (auto const& o : user_defined_overloads)
    {
        assert(o.builtin == false);
        all_overloads.insert(o);
    }

    QUX_CO_ANSWER(all_overloads);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_declaration)
{
    // TODO: Rewrite this to work.

    selection_reference const& func_addr = input;

    std::string dbg_func_name = to_string(input);

    auto functum = qualified_parent(input).value();



    assert(!qualified_is_contextual(func_addr));


    auto const &overloads = co_await QUX_CO_DEP(list_user_functum_overloads, (functum));

    auto const & declarations = co_await QUX_CO_DEP(list_user_functum_overload_declarations, (functum));

    assert(overloads.size() == declarations.size());

    for (std::size_t i = 0; i < overloads.size(); i++)
    {
        if (overloads.at(i) == input.overload)
        {
            co_return declarations.at(i);
        }

    }

    throw std::logic_error("Function declaration not found");

}