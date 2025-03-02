// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"

#include "quxlang/res/overloads.hpp"

#include "quxlang/operators.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_list_user_overload_declarations)
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

    auto decls = co_await QUX_CO_DEP(functum_list_user_overload_declarations, (input));

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

            auto type = co_await QUX_CO_DEP(lookup, (ctx_type));
            if (!type)
            {
                throw std::logic_error("Type not found");
            }

            parameter_type formal_parameter;

            formal_parameter.type = type.value();

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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_overloads)
{

    auto const & builtins = co_await QUX_CO_DEP(functum_builtin_overloads, (input_val));
    auto const & user = co_await QUX_CO_DEP(functum_user_overloads, (input_val));

    std::set<temploid_ensig> results;

    for (auto const & ol : builtins)
    {
        results.insert(ol);
    }

    for (auto const & ol : user)
    {
        results.insert(ol);
    }

    co_return results;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_declaration)
{
    // TODO: Rewrite this to work.

    type_symbol const& functum = input.templexoid;

    auto const& decl_map = co_await QUX_CO_DEP(functum_map_user_formal_ensigs, (functum));

    if (!decl_map.contains(input.which))
    {
        throw std::logic_error("Function not found");
    }

    std::size_t index = decl_map.at(input.which);

    auto const& decls = co_await QUX_CO_DEP(functum_list_user_overload_declarations, (functum));

    co_return decls.at(index);
}