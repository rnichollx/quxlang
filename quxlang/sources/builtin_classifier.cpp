//
// Created by Ryan Nicholl on 10/5/2024.
//
#include "quxlang/intrinsic_classifier.hpp"
#include "quxlang/operators.hpp"

namespace quxlang
{
    // This implements builtin operators for primitives,
    // It assumes that we have already checked that the function is builtin and are just
    // checking which implementation to use.
    template < typename Inst >
    bool implement_binary_instruction(std::optional< vmir2::vm_instruction >& out, std::string const& operator_str, bool enable_rhs, submember const& member, invotype const& call, vmir2::invocation_args const& args, bool flip = false)
    {
        if (member.name == "OPERATOR" + operator_str || (member.name == "OPERATOR" + operator_str + "RHS" && enable_rhs))
        {

            if (call.named.contains("THIS") && call.named.contains("OTHER") && args.size() == 3)
            {
                auto this_slot_id = args.named.at("THIS");
                auto other_slot_id = args.named.at("OTHER");

                Inst instr{};

                instr.a = this_slot_id;
                instr.b = other_slot_id;
                instr.result = args.named.at("RETURN");
                if (member.name == "OPERATOR" + operator_str + "RHS")
                {
                    std::swap(instr.a, instr.b);
                }

                out = instr;
                return true;
            }
        }
        return false;
    }

} // namespace quxlang

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

    auto val_parameter = argif{.type = of_type};
    auto new_parameter = argif{.type = create_nslot(of_type)};
    auto const_parameter = argif{.type = make_cref(of_type)};
    auto write_parameter = argif{.type = make_wref(of_type)};
    auto numeric_parameter = argif{.type = numeric_literal_reference{}};

    // Default CTOR
    result.push_back(signature{.ensig = temploid_ensig{ .interface = intertype{.named = {{"THIS", new_parameter}}}}});

    // Copy CTOR
    result.push_back(signature{.ensig = temploid_ensig{.interface = intertype{.named = {{"THIS", new_parameter}, {"OTHER", numeric_parameter}}}}});

    if (of_type.type_is< int_type >())
    {
        // Numeric Literal CTOR
        result.push_back(signature{.ensig = temploid_ensig{.interface = intertype{.named = {{"THIS", new_parameter}, {"OTHER", numeric_parameter}}}}});
    }

    return result;
}
std::map< std::string, quxlang::signature > quxlang::intrinsic_builtin_classifier::list_builtin_binary_operator(type_symbol of_type, std::string_view oper, bool rhs)
{
    std::map< std::string, signature > result;

    auto val_parameter = argif{.type = of_type};
    auto new_parameter = argif{.type = create_nslot(of_type)};
    auto const_parameter = argif{.type = make_cref(of_type)};
    auto write_parameter = argif{.type = make_wref(of_type)};
    auto numeric_parameter = argif{.type = numeric_literal_reference{}};

    for (auto oper : assignment_operators)
    {
        std::string name = "OPERATOR" + oper;
        signature sig;

        sig.ensig.interface.named["THIS"] = write_parameter;
        sig.ensig.interface.named["OTHER"] = val_parameter;

        result[name] = sig;
    }

    for (auto oper : compare_operators)
    {
        std::string name = "OPERATOR" + oper;
        signature sig;

        sig.ensig.interface.named["THIS"] = val_parameter;
        sig.ensig.interface.named["OTHER"] = val_parameter;
        sig.return_type = bool_type{};

        result[name] = sig;
    }

    if (of_type.type_is< bool_type >())
    {
        for (auto oper : logic_operators)
        {
            std::string name = "OPERATOR" + oper;
            signature sig;

            //sig.ensig.builtin = true;
            sig.ensig.interface.named["THIS"] = val_parameter;
            sig.ensig.interface.named["OTHER"] = val_parameter;
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

            sig.ensig.interface.named["THIS"] = val_parameter;
            sig.ensig.interface.named["OTHER"] = val_parameter;
            sig.return_type = of_type;

            result[name] = sig;
        }

        for (auto oper : bitwise_operators)
        {
            std::string name = "OPERATOR" + oper;
            signature sig;

            sig.ensig.interface.named["THIS"] = val_parameter;
            sig.ensig.interface.named["OTHER"] = val_parameter;
            sig.return_type = of_type;

            result[name] = sig;
        }
    }

    return result;
}
std::optional< quxlang::vmir2::vm_instruction > quxlang::intrinsic_builtin_classifier::intrinsic_instruction(type_symbol func, vmir2::invocation_args args)
{
    std::string funcname = to_string(func);

    if (funcname == "[4] I64::.OPERATOR[] #{@THIS CONST& [4] I64, U64}")
    {
        int debugpoint = 0;
    }
    auto cls = func_class(func);
    if (!cls)
    {
        return std::nullopt;
    }

    if (!is_intrinsic_type(*cls))
    {
        return std::nullopt;
    }

    auto instanciation = func.cast_ptr< instanciation_reference >();
    assert(instanciation);

    auto selection = &instanciation->temploid;
    assert(selection);

    auto member = selection->templexoid.cast_ptr< submember >();
    assert(member);

    auto const& call = instanciation->params;

    if (member->name == "OPERATOR??")
    {
        if (cls->template type_is< pointer_type >() && cls->as<pointer_type>().ptr_class != pointer_class::ref)
        {
            if (args.named.contains("THIS") && args.named.contains("RETURN") && args.size() == 2)
            {
                auto this_slot_id = args.named.at("THIS");

                vmir2::to_bool tb{};
                tb.from = this_slot_id;
                tb.to = args.named.at("RETURN");

                return tb;
            }
        }
    }

    if (member->name == "OPERATOR[]")
    {
        if (cls->template type_is< array_type >())
        {
            if (call.named.contains("THIS") && args.named.contains("RETURN") && call.positional.size() == 1 && args.size() == 3)
            {
                auto this_slot_id = args.named.at("THIS");
                auto index_slot_id = args.positional.at(0);
                auto return_slot_id = args.named.at("RETURN");

                vmir2::access_array aca{};
                aca.base_index = this_slot_id;
                aca.index_index = index_slot_id;
                aca.store_index = return_slot_id;

                return aca;
            }
        }
    }

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
        else if (args.size() == 1 && args.named.contains("THIS"))
        {
            vmir2::load_const_zero result;
            result.target = args.named.at("THIS");
            return result;
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
    else if (cls->template type_is< int_type >())
    {
        std::optional< vmir2::vm_instruction > instr;
        if (implement_binary_instruction< vmir2::int_add >(instr, "+", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction< vmir2::int_sub >(instr, "-", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction< vmir2::int_mul >(instr, "*", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction< vmir2::int_div >(instr, "/", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction< vmir2::int_mod >(instr, "%", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_eq>(instr, "==", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_ne>(instr, "!=", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_lt>(instr, "<", true, *member, call, args))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_lt>(instr, ">", true, *member, call, args, true))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_ge>(instr, "<=", true, *member, call, args, true))
        {
            return instr;
        }
        if (implement_binary_instruction<vmir2::cmp_ge>(instr, ">=", true, *member, call, args))
        {
            return instr;
        }
    }

    return std::nullopt;
}

bool quxlang::intrinsic_builtin_classifier::is_intrinsic_type(type_symbol of_type)
{
    return of_type.type_is< int_type >() || of_type.type_is< bool_type >() || of_type.type_is< pointer_type >() ||
           of_type.type_is< array_type >();
}