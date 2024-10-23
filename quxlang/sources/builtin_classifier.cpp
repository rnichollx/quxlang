//
// Created by Ryan Nicholl on 10/5/2024.
//
#include "quxlang/intrinsic_classifier.hpp"
#include "quxlang/operators.hpp"

std::vector< quxlang::signature > quxlang::intrinsic_builtin_classifier::list_intrinsics(type_symbol func)
{

    if (auto cls = func.cast_ptr< submember >(); cls)
    {
        if (cls->name == "CONSTRUCTOR")
        {
            return list_constructor(cls->of);
        }
    }

    return {};
}
std::vector< quxlang::signature > quxlang::intrinsic_builtin_classifier::list_constructor(type_symbol of_type)
{
    // Check if it's an intrinsic type

    std::vector< signature > result;

    if (!is_intrinsic_type(of_type))
    {
        return {};
    }

    auto val_parameter = parameter_type{.type = of_type};
    auto new_parameter = parameter_type{.type = create_nslot(of_type)};
    auto const_parameter = parameter_type{.type = make_cref(of_type)};
    auto write_parameter = parameter_type{.type = make_wref(of_type)};
    auto numeric_parameter = parameter_type{.type = numeric_literal_reference{}};

    // Default CTOR
    result.push_back(signature{.ol = overload{.builtin = true, .params = paratype{.named = {{"THIS", new_parameter}}}}});

    // Copy CTOR
    result.push_back(signature{.ol = overload{.builtin = true, .params = paratype{.named = {{"THIS", new_parameter}, {"OTHER", const_parameter}}}}});

    if (of_type.type_is< int_type >())
    {
        // Numeric Literal CTOR
        result.push_back(signature{.ol = overload{.builtin = true, .params = paratype{.named = {{"THIS", new_parameter}, {"OTHER", numeric_parameter}}}}});
    }

    return result;
}
std::map< std::string, quxlang::signature > quxlang::intrinsic_builtin_classifier::list_builtin_binary_operator(type_symbol of_type, std::string_view oper, bool rhs)
{
    std::map< std::string, signature > result;

    auto val_parameter = parameter_type{.type = of_type};
    auto new_parameter = parameter_type{.type = create_nslot(of_type)};
    auto const_parameter = parameter_type{.type = make_cref(of_type)};
    auto write_parameter = parameter_type{.type = make_wref(of_type)};
    auto numeric_parameter = parameter_type{.type = numeric_literal_reference{}};

    for (auto oper : assignment_operators)
    {
        std::string name = "OPERATOR" + oper;
        signature sig;

        sig.ol.builtin = true;
        sig.ol.params.named["THIS"] = write_parameter;
        sig.ol.params.named["OTHER"] = val_parameter;

        result[name] = sig;
    }

    for (auto oper : bool_operators)
    {
        std::string name = "OPERATOR" + oper;
        signature sig;

        sig.ol.builtin = true;
        sig.ol.params.named["THIS"] = val_parameter;
        sig.ol.params.named["OTHER"] = val_parameter;
        sig.return_type = bool_type{};

        result[name] = sig;
    }

    if (of_type.type_is< bool_type >())
    {
        for (auto oper : logic_operators)
        {
            std::string name = "OPERATOR" + oper;
            signature sig;

            sig.ol.builtin = true;
            sig.ol.params.named["THIS"] = val_parameter;
            sig.ol.params.named["OTHER"] = val_parameter;
            sig.return_type = bool_type{};

            result[name] = sig;
        }
    }

    if (of_type.type_is< int_type >())
    {
        for (auto oper : arithmetic_operators)
        {
            std::string name = "OPERATOR" + oper;
            signature sig;

            sig.ol.builtin = true;
            sig.ol.params.named["THIS"] = val_parameter;
            sig.ol.params.named["OTHER"] = val_parameter;
            sig.return_type = of_type;

            result[name] = sig;
        }

        for (auto oper : bitwise_operators)
        {
            std::string name = "OPERATOR" + oper;
            signature sig;

            sig.ol.builtin = true;
            sig.ol.params.named["THIS"] = val_parameter;
            sig.ol.params.named["OTHER"] = val_parameter;
            sig.return_type = of_type;

            result[name] = sig;
        }
    }

    return result;
}
std::optional< quxlang::vmir2::vm_instruction > quxlang::intrinsic_builtin_classifier::intrinsic_instruction(type_symbol func, vm_invocation_args args)
{
    return std::nullopt;
}
bool quxlang::intrinsic_builtin_classifier::is_intrinsic_type(type_symbol of_type)
{
    return of_type.type_is< int_type >() || of_type.type_is< bool_type >() || of_type.type_is< instance_pointer_type >();
}