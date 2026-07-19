// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/builtin_vm_procedure3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>

#include <map>
#include <optional>
#include <string>

rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > quxlang::builtin_vm_procedure3_impl(instanciation_reference input)
{
    auto parse_type_symbol_text = [](std::string const& text) -> type_symbol
    {
        auto ctx = parsers::make_unlocated_parsing_context(text);
        auto result = parsers::parse_type_symbol(ctx);
        if (ctx.iter_pos != ctx.iter_end)
        {
            throw quxlang::compiler_bug("Input not fully parsed");
        }
        return result;
    };

    auto make_builtin_pattern = [&](std::string member_name, instatype params) -> type_symbol
    {
        return instanciation_reference{
            .temploid = temploid_reference{
                .templexoid = submember{
                    .of = parse_type_symbol_text("TT(t1)"),
                    .name = std::move(member_name),
                },
            },
            .params = std::move(params),
        };
    };
    auto matches_builtin_pattern = [&](type_symbol const& pattern) -> std::optional< template_match_results >
    {
        auto normalized_input = input;
        normalized_input.temploid.overload_id = std::nullopt;
        return match_template2(pattern, normalized_input);
    };

    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});

    std::optional< qualifier > same_type_constructor_source_qualifier;
    if (typeis< submember >(input.temploid.templexoid) &&
        as< submember >(input.temploid.templexoid).name == "CONSTRUCTOR")
    {
        std::map< std::string, parameter_instantiation >::const_iterator const this_parameter = input.params.named.find("THIS");
        std::map< std::string, parameter_instantiation >::const_iterator const other_parameter = input.params.named.find("OTHER");
        if (this_parameter != input.params.named.end() && other_parameter != input.params.named.end())
        {
            type_symbol const& this_type = parameter_instantiation_type(this_parameter->second);
            type_symbol const& other_type = parameter_instantiation_type(other_parameter->second);
            if (typeis< nvalue_slot >(this_type) && typeis< ptrref_type >(other_type) &&
                as< ptrref_type >(other_type).ptr_class == pointer_class::ref &&
                remove_ref(this_type) == remove_ref(other_type))
            {
                same_type_constructor_source_qualifier = as< ptrref_type >(other_type).qual;
            }
        }
    }

    auto ctor_match = make_builtin_pattern("CONSTRUCTOR", instatype{
        .named = {{"THIS", make_type_instantiation(parse_type_symbol_text("NEW& TT(t1)"))}},
    });


    auto input_str = quxlang::to_string(input);

    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(ctor_match));

    auto template_match_result = matches_builtin_pattern(ctor_match);

    if (template_match_result)
    {
        co_return co_await rpnx::querygraph::request< builtin_default_ctor_vm_procedure3_query >(input);
    }
    else if (matches_builtin_pattern(make_builtin_pattern("DESTRUCTOR", instatype{
        .named = {{"THIS", make_type_instantiation(parse_type_symbol_text("DESTROY& TT(t1)"))}},
    })))
    {
        auto result =  co_await rpnx::querygraph::request< builtin_dtor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (same_type_constructor_source_qualifier == qualifier::temp)
    {
        auto result = co_await rpnx::querygraph::request< builtin_move_ctor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (same_type_constructor_source_qualifier == qualifier::constant)
    {
        auto result = co_await rpnx::querygraph::request< builtin_copy_ctor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (typeis< submember >(input.temploid.templexoid) && as< submember >(input.temploid.templexoid).name == "CONSTRUCTOR")
    {
        submember const& member = as< submember >(input.temploid.templexoid);
        class_kind const parent_kind = co_await rpnx::querygraph::request< class_type_query >(member.of);
        if (parent_kind == class_kind::union_ || parent_kind == class_kind::variant)
        {
            co_return co_await rpnx::querygraph::request< builtin_default_ctor_vm_procedure3_query >(input);
        }
    }
    else if (matches_builtin_pattern(make_builtin_pattern("OPERATOR<->", instatype{
        .named = {
            {"THIS", make_type_instantiation(parse_type_symbol_text("& AUTO(t1)"))},
            {"OTHER", make_type_instantiation(parse_type_symbol_text("& AUTO(t1)"))},
        },
    })))
    {
        auto result = co_await rpnx::querygraph::request< builtin_swap_vm_procedure3_query >(input);
        co_return result;
    }
    else if (matches_builtin_pattern(make_builtin_pattern("OPERATOR<=>", instatype{
        .named = {
            {"THIS", make_type_instantiation(parse_type_symbol_text("CONST& AUTO(t1)"))},
            {"OTHER", make_type_instantiation(parse_type_symbol_text("CONST& AUTO(t1)"))},
        },
    })))
    {
        type_symbol const& compared_type = input.temploid.templexoid.get_as< submember >().of;
        class_kind const compared_kind = co_await rpnx::querygraph::request< class_type_query >(compared_type);
        if (compared_kind == class_kind::enum_ || compared_kind == class_kind::flagset)
        {
            co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > > gen(machine_info, input);
            co_return co_await gen.co_generate_builtin_nominal_integer_spaceship(input);
        }
    }
    else if (matches_builtin_pattern(make_builtin_pattern("OPERATOR==", instatype{
        .named = {
            {"THIS", make_type_instantiation(parse_type_symbol_text("CONST& AUTO(t1)"))},
            {"OTHER", make_type_instantiation(parse_type_symbol_text("CONST& AUTO(t1)"))},
        },
    })))
    {
        auto const& compared_type = input.temploid.templexoid.get_as< submember >().of;
        if (co_await rpnx::querygraph::request< symbol_type_query >(compared_type) == symbol_kind::class_ && co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(compared_type))
        {
            co_return co_await rpnx::querygraph::request< builtin_datatype_compare_vm_procedure3_query >(input);
        }
    }
    if (typeis< builtin_symbol >(input.temploid.templexoid))
    {
        auto const& builtin = as< builtin_symbol >(input.temploid.templexoid);
        co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > > gen(machine_info, input);
        if (builtin.name == "SERIALIZE_UINTANY")
        {
            co_return co_await gen.co_generate_builtin_serialize_uintany(input);
        }
        if (builtin.name == "DESERIALIZE_UINTANY")
        {
            co_return co_await gen.co_generate_builtin_deserialize_uintany(input);
        }
        if (builtin.name == "SERIALIZE_LEB128")
        {
            co_return co_await gen.co_generate_builtin_serialize_leb128(input);
        }
        if (builtin.name == "DESERIALIZE_LEB128")
        {
            co_return co_await gen.co_generate_builtin_deserialize_leb128(input);
        }
    }

    if (typeis< subsymbol >(input.temploid.templexoid))
    {
        subsymbol const& ss = as< subsymbol >(input.temploid.templexoid);
        if (ss.name == "GET_INTERFACE_IMPL" && co_await rpnx::querygraph::request< symbol_type_query >(ss.of) == symbol_kind::implementation_)
        {
            co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > > gen(machine_info, input);
            co_return co_await gen.co_generate_interface_get_impl(input);
        }
    }

    if (typeis< submember >(input.temploid.templexoid))
    {
        co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > > gen(machine_info, input);
        auto const & sm = as< submember >(input.temploid.templexoid);
        auto parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(sm.of);

        if (parent_kind == symbol_kind::interface_)
        {
            co_return co_await gen.co_generate_interface_builtin(input);
        }
       
        if (parent_kind == symbol_kind::global_variable)
        {
            if (sm.name == "GET_REFERENCE")
            {
                co_return co_await gen.co_generate_builtin_global_get_reference(input);
            }
            else if (sm.name == "INIT")
            {
                co_return co_await gen.co_generate_builtin_global_init(input);
            }
        }
        if (typeis< constexpr_proxy >(sm.of) && (sm.name == "OPERATOR++" || sm.name == "OPERATOR->" || sm.name == "OPERATOR:="))
        {
            throw compiler_bug("__CONSTEXPR_PROXY " + sm.name + " is a builtin intrinsic and has no generated VM procedure");
        }
        if (sm.name == "CONSTRUCTOR" && typeis< array_type >(sm.of))
        {
            co_return co_await rpnx::querygraph::request< builtin_default_ctor_vm_procedure3_query >(input);
        }
        if (matches_builtin_pattern(make_builtin_pattern("OPERATOR:=", instatype{
            .named = {
                {"THIS", make_type_instantiation(parse_type_symbol_text("WRITE& AUTO(t1)"))},
                {"OTHER", make_type_instantiation(parse_type_symbol_text("AUTO(t1)"))},
            },
        })))
        {
            auto result = co_await rpnx::querygraph::request< builtin_assignment_vm_procedure3_query >(input);
            co_return result;
        }
        if (sm.name == "BEGIN")
        {
            co_return co_await gen.co_generate_builtin_access_member(input, "__start");
        }
        else if (sm.name == "END")
        {
            co_return co_await gen.co_generate_builtin_access_member(input, "__end");
        }
        else if (sm.name == "SERIALIZE")
        {
            co_return co_await gen.co_generate_builtin_serialize(input);
        }
        else if (sm.name == "DESERIALIZE")
        {
            co_return co_await gen.co_generate_builtin_deserialize(input);
        }

    }


    throw compiler_bug("generation of builtin functanoid '" + input_str + "' is not implemented, no intrinsic routine generator matched");
}
