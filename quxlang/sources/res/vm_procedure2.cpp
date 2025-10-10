// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_codegen_emitter.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/res/expr/co_vmir_routine_emitter.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/res/vm_procedure2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>




QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure3)
{
    assert(!type_is_contextual(input));
    if (co_await QUX_CO_DEP(function_builtin, (input.temploid)))
    {
        co_return co_await QUX_CO_DEP(builtin_vm_procedure3, (input));
    }
    else
    {
        co_return co_await QUX_CO_DEP(user_vm_procedure3, (input));
    }
}




QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_functanoid(input);
}



QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_vm_procedure3)
{
    auto ctor_match = quxlang::parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& TT(t1)}");


    auto input_str = quxlang::to_string(input);

    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(ctor_match));

    auto template_match_result = match_template2(ctor_match, input);

    if (template_match_result)
    {
        co_return co_await QUX_CO_DEP(builtin_default_ctor_vm_procedure3, (input));
    }
    else if (match_template2(quxlang::parsers::parse_type_symbol("TT(t1)::.DESTRUCTOR#{ @THIS DESTROY& TT(t1)}"), input))
    {
        auto result =  co_await QUX_CO_DEP(builtin_dtor_vm_procedure3, (input));
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& AUTO(t1), @OTHER CONST& AUTO(t1)}"), input))
    {
        auto result = co_await QUX_CO_DEP(builtin_copy_ctor_vm_procedure3, (input));
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& AUTO(t1), @OTHER TEMP& AUTO(t1)}"), input))
    {
        auto result = co_await QUX_CO_DEP(builtin_move_ctor_vm_procedure3, (input));
        co_return result;
    }
    else if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR<-> #{@THIS & AUTO(t1), @OTHER & AUTO(t1)}"), input))
    {
        auto result = co_await QUX_CO_DEP(builtin_swap_vm_procedure3, (input));
        co_return result;
    }

    if (match_template2(parsers::parse_type_symbol("TT(t1)::.OPERATOR:= #{@THIS WRITE& AUTO(t1), @OTHER AUTO(t1)}"), input))
    {
        auto result = co_await QUX_CO_DEP(builtin_assignment_vm_procedure3, (input));
        co_return result;
    }

    if (typeis< submember >(input.temploid.templexoid))
    {
        co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);
        auto const & sm = as< submember >(input.temploid.templexoid);
        if (sm.name == "BEGIN")
        {
            co_return co_await gen.co_generate_builtin_access_member(input, "__start");
        }
        else if (sm.name == "END")
        {
            co_return co_await gen.co_generate_builtin_access_member(input, "__end");
        }

    }

    throw compiler_bug("generation of builtin routine \'" + input_str + "\' not implemented");
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_default_ctor_vm_procedure3)
{
    std::string input_name = quxlang::to_string(input);
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_ctor(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_copy_ctor_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_copy_ctor(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_move_ctor_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_move_ctor(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_assignment_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_assignment(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_swap_vm_procedure3)
{
   // throw compiler_bug("not implemented");
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_swap(input);
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_dtor_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_dtor(input);
}