// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/builtin_vm_procedure3_spec.hpp>
#include <quxlang/queries/machine_info.hpp>

#include <quxlang/co_vmir_generator2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>





rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > quxlang::builtin_vm_procedure3_impl(instanciation_reference input)
{
    auto const machine_info = co_await rpnx::querygraph::request< machine_info_query >(machine_info_query::input_type{});

    auto ctor_match = quxlang::parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& TT(t1)}");


    auto input_str = quxlang::to_string(input);

    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(ctor_match));

    auto template_match_result = match_template2(ctor_match, input);

    if (template_match_result)
    {
        co_return co_await rpnx::querygraph::request< builtin_default_ctor_vm_procedure3_query >(input);
    }
    else if (match_template2(quxlang::parsers::parse_type_symbol("TT(t1)::.DESTRUCTOR#{ @THIS DESTROY& TT(t1)}"), input))
    {
        auto result =  co_await rpnx::querygraph::request< builtin_dtor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& AUTO(t1), @OTHER CONST& AUTO(t1)}"), input))
    {
        auto result = co_await rpnx::querygraph::request< builtin_copy_ctor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& AUTO(t1), @OTHER TEMP& AUTO(t1)}"), input))
    {
        auto result = co_await rpnx::querygraph::request< builtin_move_ctor_vm_procedure3_query >(input);
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR<-> #{@THIS & AUTO(t1), @OTHER & AUTO(t1)}"), input))
    {
        auto result = co_await rpnx::querygraph::request< builtin_swap_vm_procedure3_query >(input);
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR== #{@THIS CONST& AUTO(t1), @OTHER CONST& AUTO(t1)}"), input))
    {
        auto const& compared_type = input.temploid.templexoid.get_as< submember >().of;
        if (co_await rpnx::querygraph::request< symbol_type_query >(compared_type) == symbol_kind::class_ && co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(compared_type))
        {
            co_return co_await rpnx::querygraph::request< builtin_datatype_compare_vm_procedure3_query >(input);
        }
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR!= #{@THIS CONST& AUTO(t1), @OTHER CONST& AUTO(t1)}"), input))
    {
        auto const& compared_type = input.temploid.templexoid.get_as< submember >().of;
        if (co_await rpnx::querygraph::request< symbol_type_query >(compared_type) == symbol_kind::class_ && co_await rpnx::querygraph::request< type_is_implicitly_datatype_query >(compared_type))
        {
            co_return co_await rpnx::querygraph::request< builtin_datatype_compare_vm_procedure3_query >(input);
        }
    }

    if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR:= #{@THIS WRITE& AUTO(t1), @OTHER AUTO(t1)}"), input))
    {
        auto result = co_await rpnx::querygraph::request< builtin_assignment_vm_procedure3_query >(input);
        co_return result;
    }

    if (typeis< submember >(input.temploid.templexoid))
    {
        co_vmir_generator2< rpnx::querygraph::coroutine< quxlang::builtin_vm_procedure3_spec > > gen(machine_info, input);
        auto const & sm = as< submember >(input.temploid.templexoid);
        auto parent_kind = co_await rpnx::querygraph::request< symbol_type_query >(sm.of);
       
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
