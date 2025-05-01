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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_initialize)
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

QUX_CO_RESOLVER_IMPL_FUNC_DEF(list_primitive_constructors)
{
    std::set< builtin_function_info > result;

    if (typeis< int_type >(input) || input.type_is< bool_type >() || input.type_is< pointer_type >())
    {

        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = make_cref(input)}}}}}, .return_type = void_type{}});
        if (input.type_is< int_type >())
        {
            result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = numeric_literal_reference{}}}}}}, .return_type = void_type{}});
        }
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{.type = create_nslot(input)}}}}}, .return_type = void_type{}});
        co_return (result);
    }

    bool should_autogen_constructor = co_await QUX_CO_DEP(class_requires_gen_default_ctor, (input));
    bool should_autogen_copy_constructor = true;

    // co_await QUX_CO_DEP(class_should_autogen_default_constructor, (input));

    if (should_autogen_constructor)
    {
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{.type = create_nslot(input)}}}}}, .return_type = void_type{}});
        co_return result;
    }

    if (should_autogen_copy_constructor)
    {
        result.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{.type = create_nslot(input)}}, {"OTHER", argif{.type = make_cref(input)}}}}}, .return_type = void_type{}});
    }

    co_return result;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(functum_primitive_overloads)
{
    auto const& functum = input;
    std::optional< type_symbol > parent_opt;
    std::string input_name = to_string(input);

    std::optional< ast2_class_declaration > class_ent;

    if (!typeis< submember >(functum))
    {
        co_return {};
    }

    submember const& as_submember = as< submember >(functum);
    type_symbol const& parent = as_submember.of;

    std::string const& name = as_submember.name;

    std::set< builtin_function_info > allowed_operations;

    if (name == "CONSTRUCTOR")
    {
        co_return co_await QUX_CO_DEP(list_primitive_constructors, (parent));
    }

    if (name == "OPERATOR??" && (parent.test< pointer_type >(
                                     [](pointer_type p)
                                     {
                                         return p.ptr_class != pointer_class::ref;
                                     }) ||
                                 parent.type_is< int_type >()))
    {
        builtin_function_info bl_info;

        bl_info.overload = temploid_ensig{.interface = {.named = {{"THIS", {parent}}}}};

        bl_info.return_type = bool_type{};

        allowed_operations.insert(bl_info);
    }

    if (name == "OPERATOR[]" && parent.type_is< array_type >())
    {

        static std::vector< qualifier > quals{qualifier::mut, qualifier::constant, qualifier::mut, qualifier::temp, qualifier::write};

        for (qualifier qv : quals)
        {
            // TODO: should this always be 64-bit?
            builtin_function_info br_info;
            br_info.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{pointer_type{.target = parent, .ptr_class = pointer_class::ref, .qual = qualifier::constant}}}}, .positional = {argif{.type = int_type{.bits = 64}}}}};
            br_info.return_type = pointer_type{.target = parent.get_as< array_type >().element_type, .ptr_class = pointer_class::ref, .qual = qv};

            allowed_operations.insert(br_info);
        }
    }

    if (name.starts_with("OPERATOR"))
    {
        std::string operator_name = name.substr(8);
        bool is_rhs = false;

        if (operator_name.ends_with("RHS"))
        {
            operator_name = operator_name.substr(0, operator_name.size() - 3);
            is_rhs = true;
        }

        bool is_int_type = typeis< int_type >(parent);
        bool is_bool_type = typeis< bool_type >(parent);
        bool is_pointer_type = typeis< pointer_type >(parent);
        bool is_arithmetic_operator = arithmetic_operators.contains(operator_name);
        bool is_assignment_operator = assignment_operators.contains(operator_name);
        bool is_compare_operator = compare_operators.contains(operator_name);
        bool is_incdec_operator = incdec_operators.contains(operator_name);
        bool is_pointer_arith_operator = pointer_arithmetic_operators.contains(operator_name);

        if (is_int_type)
        {
            if (is_arithmetic_operator)
            {
                allowed_operations.insert(builtin_function_info{.overload =
                                                                    temploid_ensig{
                                                                        .interface =
                                                                            intertype{
                                                                                .named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}},
                                                                            },
                                                                    },
                                                                .return_type = parent});
            }
            else if (is_compare_operator)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}}}}, .return_type = bool_type{}});
            }
            else if (is_incdec_operator && !is_rhs)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS",  argif{make_mref(parent)}}}}}, .return_type = parent});
            }
            else if (is_incdec_operator && is_rhs)
            {
                allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = intertype{.named = {{"THIS",  argif{make_mref(parent)}}}}}, .return_type = make_mref(parent)});
            }
        }

        if (is_assignment_operator)
        {
            if (is_rhs)
            {
                // no primitive RHS assignments exist.
                co_return {};
            }
            else
            {
                allowed_operations.insert(builtin_function_info{
                    .overload = temploid_ensig{.interface =
                                                   intertype{
                                                       .named = {{"THIS", argif{make_wref(parent)}}, {"OTHER", argif{parent}}},
                                                   }},
                    .return_type = void_type{},
                });
            }
        }

        if (typeis< int_type >(parent) && compare_operators.contains(operator_name))
        {
            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface =
                                                                                           intertype{
                                                                                               .named = {{"THIS", argif{parent}}, {"OTHER", argif{parent}}},
                                                                                           }},
                                                            .return_type = bool_type{}});
        }

        if (typeis< numeric_literal_reference >(parent) && arithmetic_operators.contains(operator_name))
        {
            std::set< builtin_function_info > allowed_operations;

            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{numeric_literal_reference{}}}, {"OTHER", argif{numeric_literal_reference{}}}}}}, .return_type = numeric_literal_reference{}});
            co_return (allowed_operations);
        }

        if (typeis< pointer_type >(parent) && operator_name == rightarrow_operator)
        {
            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{parent}}}}}, .return_type = make_mref(remove_ptr(parent))});
        }
        if (typeis <pointer_type>(parent) && operator_name == "+")
        {
            auto uintptr_type = co_await QUX_CO_DEP(uintpointer_type, (std::monostate{}));
            allowed_operations.insert(builtin_function_info{.overload = temploid_ensig{.interface = {.named = {{"THIS", argif{parent}}, {"OTHER", argif{uintptr_type}}}}}, .return_type = parent});
        }

        co_return (allowed_operations);
    }

    co_return allowed_operations;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_builtin)
{
    auto builtin_overloads = co_await QUX_CO_DEP(functum_builtin_overloads, (input_val.templexoid));

    for (auto const& info : builtin_overloads)
    {
        if (info == input_val.which)
        {
            co_return true;
        }
    }

    co_return false;
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(function_primitive)
{
    auto primitive_overloads = co_await QUX_CO_DEP(functum_primitive_overloads, (input_val.templexoid));

    for (auto const& info : primitive_overloads)
    {
        if (info.overload == input.which)
        {
            co_return info;
        }
    }

    co_return std::nullopt;
}
