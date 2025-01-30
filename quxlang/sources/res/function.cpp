// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"
#include "quxlang/manipulators/argmanip.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/res/list_builtin_functum_overloads_resolver.hpp"
#include "quxlang/variant_utils.hpp"
#include "rpnx/debug.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/macros.hpp>
#include <quxlang/res/function.hpp>

using namespace quxlang;

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_positional_parameter_names)
{
    std::vector< std::string > result;
    auto const& func = co_await QUX_CO_DEP(function_declaration, (input_val));

    if (!func.has_value())
    {
        throw std::logic_error("Function not found");
    }

    std::set< std::string > names;

    for (auto const& param : func->header.call_parameters)
    {
        if (param.api_name.has_value())
        {
            // non-positional parameter
            continue;
        }

        if (names.contains(param.name))
        {
            throw std::logic_error("Duplicate parameter name");
        }

        result.push_back(param.name);
    }

    QUX_CO_ANSWER(result);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_instanciation)
{
    auto selection = co_await QUX_CO_DEP(functum_select_function, (input_val));

    if (!selection)
    {
        QUX_WHY("No function found that matches the given parameters.");

        QUX_CO_ANSWER(std::nullopt);
        // throw std::logic_error("No function found that matches the given parameters.");
    }

    co_return co_await QUX_CO_DEP(function_instanciation, (initialization_reference{.initializee = selection.value(), .parameters = input_val.parameters}));
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_builtin_constructors)
{
    std::set< primitive_function_info > result;

    if (typeis< int_type >(input) || input.type_is< bool_type >() || input.type_is< pointer_type >())
    {

        result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", create_nslot(input)}, {"OTHER", make_cref(input)}}}}, .return_type = void_type{}});
        if (input.type_is< int_type >())
        {
            result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", create_nslot(input)}, {"OTHER", numeric_literal_reference{}}}}}, .return_type = void_type{}});
        }
        result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", create_nslot(input)}}}}, .return_type = void_type{}});
        co_return (result);
    }

    bool should_autogen_constructor = co_await QUX_CO_DEP(class_should_autogen_default_constructor, (input));
    bool should_autogen_copy_constructor = true;

    // co_await QUX_CO_DEP(class_should_autogen_default_constructor, (input));

    if (should_autogen_constructor)
    {
        result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = intertype{.named = {{"THIS", create_nslot(input)}}}}, .return_type = input});
        co_return result;
    }

    if (should_autogen_copy_constructor)
    {
        result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = intertype{.named = {{"THIS", create_nslot(input)}, {"OTHER", make_cref(input)}}}}, .return_type = void_type{}});
    }

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_builtin_functum_overloads)
{
    auto const& functum = input;
    std::optional< type_symbol > parent_opt;
    std::string name = to_string(functum);

    std::optional< ast2_class_declaration > class_ent;

    if (typeis< submember >(functum))
    {
        auto parent = as< submember >(functum).of;

        bool builtin_primitive_type = false;

        if (typeis< int_type >(parent))
        {
            builtin_primitive_type = true;
        }
        else if (typeis< pointer_type >(parent))
        {
            builtin_primitive_type = true;
        }
        else if (typeis< bool_type >(parent))
        {
            builtin_primitive_type = true;
        }

        auto parent_class_exists = co_await *c->lk_exists(parent);
        if (parent_class_exists)
        {
            auto decl = co_await QUX_CO_DEP(symboid, (parent));

            if (typeis< ast2_class_declaration >(decl))
            {
                class_ent = as< ast2_class_declaration >(decl);
            }
        }

        parent_opt = parent;
        auto name = as< submember >(functum).name;

        std::set< primitive_function_info > allowed_operations;

        if (name.starts_with("OPERATOR"))
        {
            std::string operator_name = name.substr(8);
            bool is_rhs = false;
            if (operator_name.ends_with("RHS"))
            {
                operator_name = operator_name.substr(0, operator_name.size() - 3);

                is_rhs = true;
            }

            if (typeis< int_type >(parent) && arithmetic_operators.contains(operator_name))
            {
                allowed_operations.insert(primitive_function_info{.overload =
                                                                      temploid_formal_intertype{
                                                                          .builtin = true,
                                                                          .interface =
                                                                              intertype{
                                                                                  .named = {{"THIS", parent}, {"OTHER", parent}},
                                                                              },
                                                                      },
                                                                  .return_type = parent});
            }

            if (assignment_operators.contains(operator_name) && is_rhs)
            {
                // Cannot assign from RHS for now
                // TODO: Change this?
                co_return {};
            }

            if (assignment_operators.contains(operator_name))
            {
                allowed_operations.insert(primitive_function_info{
                    .overload = temploid_formal_intertype{.builtin = true,
                                                  .interface =
                                                      intertype{
                                                          .named = {{"THIS", make_wref(parent)}, {"OTHER", parent}},
                                                      }},
                    .return_type = void_type{},
                });
            }

            if (typeis< int_type >(parent) && bool_operators.contains(operator_name))
            {
                allowed_operations.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true,
                                                                                                .interface =
                                                                                                    intertype{
                                                                                                        .named = {{"THIS", parent}, {"OTHER", parent}},
                                                                                                    }},
                                                                  .return_type = bool_type{}});
            }

            if (typeis< numeric_literal_reference >(parent) && arithmetic_operators.contains(operator_name))
            {
                std::set< primitive_function_info > allowed_operations;

                allowed_operations.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", numeric_literal_reference{}}, {"OTHER", numeric_literal_reference{}}}}}, .return_type = numeric_literal_reference{}});
                co_return (allowed_operations);
            }

            if (typeis< pointer_type >(parent) && operator_name == rightarrow_operator)
            {
                allowed_operations.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", parent}}}}, .return_type = make_mref(remove_ptr(parent))});
            }

            co_return (allowed_operations);
        }

        else if (name == "CONSTRUCTOR")
        {
           co_return co_await QUX_CO_DEP(list_builtin_constructors, (parent));
        }

            else if (name == "DESTRUCTOR")
            {
                std::set< primitive_function_info > result;
                result.insert(primitive_function_info{.overload = temploid_formal_intertype{.builtin = true, .interface = {.named = {{"THIS", make_mref(parent)}}}}, .return_type = void_type{}});
                co_return (result);
            }
    }

    co_return {};
}