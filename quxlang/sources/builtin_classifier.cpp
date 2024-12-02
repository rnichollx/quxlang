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
std::optional< quxlang::vmir2::vm_instruction > quxlang::intrinsic_builtin_classifier::intrinsic_instruction(type_symbol func, vmir2::invocation_args args)
{
    std::string funcname = to_string(func);

    auto cls = func_class(func);
    if (!cls)
    {
        return std::nullopt;
    }

    if (!is_intrinsic_type(*cls))
    {
        return std::nullopt;
    }

    auto instanciation = func.cast_ptr< instantiation_type >();
    assert(instanciation);

    auto selection = instanciation->callee.cast_ptr< selection_reference >();
    assert(selection);

    auto member = selection->templexoid.cast_ptr< submember >();
    assert(member);

    auto const& call = instanciation->parameters;

    if (member->name == "CONSTRUCTOR")
    {
        if (call.named.contains("OTHER"))
        {
            auto const& other = call.named.at("OTHER");
            if (cls->template type_is< int_type >() && other.type_is< numeric_literal_reference >())
            {
                auto other_slot_id = args.named.at("OTHER");

                auto const& other_slot = state_.slots->slots.at(other_slot_id);

                assert(other_slot.type == numeric_literal_reference{});
                assert(other_slot.kind == vmir2::slot_kind::literal);
                assert(other_slot.literal_value.has_value());
                auto other_slot_value = other_slot.literal_value.value();

                vmir2::load_const_int result;
                result.value = other_slot_value;
                result.target = args.named.at("THIS");

                return result;
            }
            else if (other == make_cref(*cls))
            {
                auto other_slot_id = args.named.at("OTHER");
                auto this_slot_id = args.named.at("THIS");

                vmir2::load_from_ref lfr{};
                lfr.from_reference = other_slot_id;
                lfr.to_value = this_slot_id;

                return lfr;
            }
        }
        else if (cls->type_is<int_type>())
        {
            if (args.size() == 1 && args.named.contains("THIS"))
            {
                vmir2::load_const_int result;
                result.value = "0";
                result.target = args.named.at("THIS");
                return result;
            }
        }
    }

    if (member->name == "OPERATOR:=")
    {
        if (cls->template type_is< int_type >() || cls->template type_is< bool_type >() || cls->template type_is< pointer_type >())
        {
            if (call.named.contains("OTHER") && call.named.contains("THIS") && call.size() == 2)
            {
                auto const& other = call.named.at("OTHER");
                auto const& this_ = call.named.at("THIS");

                if ((other == *cls) && this_ == make_wref(*cls))
                {
                    auto other_slot_id = args.named.at("OTHER");
                    auto this_slot_id = args.named.at("THIS");

                    vmir2::store_to_ref mov{};
                    mov.from_value = other_slot_id;
                    mov.to_reference = this_slot_id;

                    return mov;
                }
            }
        }
    }
    else if (member->name == "OPERATOR->")
    {
        if (cls->template type_is< pointer_type >())
        {
            if (call.named.contains("THIS") && args.size() == 2)
            {

                auto this_slot_id = args.named.at("THIS");

                vmir2::dereference_pointer deref{};
                deref.from_pointer = this_slot_id;
                deref.to_reference = args.named.at("RETURN");

                return deref;
            }
        }
    }

    return std::nullopt;
}
bool quxlang::intrinsic_builtin_classifier::is_intrinsic_type(type_symbol of_type)
{
    return of_type.type_is< int_type >() || of_type.type_is< bool_type >() || of_type.type_is< pointer_type >();
}