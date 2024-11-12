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


    std::vector<paratype> result;

    for (auto const &decl : decls)
    {
        paratype p;



        for (auto const & param : decl.header.call_parameters)
        {
            contextual_type_reference ctx_type{.context = input, .type = param.type};

            auto type = co_await QUX_CO_DEP(canonical_symbol_from_contextual_symbol, (ctx_type));


            parameter_type formal_parameter;

            formal_parameter.type = type;
            formal_parameter.default_value = param.default_expr;

            if (param.api_name)
            {
// TODO: This part
            }

            // TODO: Also this part

            throw rpnx::unimplemented();


        }

        bool is_member_func = typeis<submember>(input);
        bool is_constructor = is_member_func && input.get_as<submember>().name == "CONSTRUCTOR";
        bool is_destructor = is_member_func && input.get_as<submember>().name == "DESTRUCTOR";


        if (is_member_func && !p.named.contains("THIS"))
        {
            if (!is_constructor && !is_destructor)
            {
                parameter_type default_this{};
                // By default, AUTO& THISTYPE
                default_this.type = auto_reference{ .target = thistype{} };
                p.named["THIS"] = default_this;
            }
            else if (is_constructor)
            {
                parameter_type default_this{};
                default_this.type = nvalue_slot{ .target = thistype{} };

                p.named["THIS"] = default_this;
            }
            else
            {
                parameter_type default_this{};
                default_this.type = dvalue_slot{ .target = thistype{} };
                p.named["THIS"] = default_this;
            }
        }
    }
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_functum_overloads)
{
    QUX_CO_GETDEP(builtins, list_builtin_functum_overloads, (input));
    QUX_CO_GETDEP(user_defined, list_user_functum_overload_declarations, (input));

    std::string name = to_string(input);
    std::set< function_overload > all_overloads;
    for (auto const& o : builtins)
    {
        all_overloads.insert(o.overload);
    }
    for (auto const& o : user_defined)
    {
        // TODO: Finish this
        throw rpnx::unimplemented();
        // all_overloads.insert(o);
    }

    QUX_CO_ANSWER(all_overloads);
}