// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_codegen_emitter.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/res/expr/co_vmir_routine_emitter.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/res/vm_procedure2.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>


QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    throw compiler_bug("removed");

    if (co_await QUX_CO_DEP(function_builtin, (input.temploid)))
    {
        co_return co_await QUX_CO_DEP(builtin_vm_procedure2, (input));
    }
    else
    {
        co_return co_await QUX_CO_DEP(user_vm_procedure2, (input));
    }
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure3)
{
    if (co_await QUX_CO_DEP(function_builtin, (input.temploid)))
    {
        co_return co_await QUX_CO_DEP(builtin_vm_procedure3, (input));
    }
    else
    {
        co_return co_await QUX_CO_DEP(user_vm_procedure3, (input));
    }
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate();
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_functanoid(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_ctor_vm_procedure2)
{
    throw compiler_bug("removed");
    std::string input_name = quxlang::to_string(input);
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate_builtin_ctor();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_vm_procedure2)
{
    throw compiler_bug("removed");
    auto ctor_match = quxlang::parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& TT(t1)}");


    auto input_str = quxlang::to_string(input);

    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(ctor_match));

    auto template_match_result = match_template2(ctor_match, input);

    if (template_match_result)
    {
        co_return co_await QUX_CO_DEP(builtin_ctor_vm_procedure2, (input));
    }
    else if (match_template2(quxlang::parsers::parse_type_symbol("TT(t1)::.DESTRUCTOR#{ @THIS DESTROY& TT(t1)}"), input))
    {
        auto result =  co_await QUX_CO_DEP(builtin_dtor_vm_procedure2, (input));
        co_return result;
    }

    throw compiler_bug("not implemented or bug");
}



QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_vm_procedure3)
{
    auto ctor_match = quxlang::parsers::parse_type_symbol("TT(t1)::.CONSTRUCTOR#{@THIS NEW& TT(t1)}");


    auto input_str = quxlang::to_string(input);

    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(ctor_match));

    auto template_match_result = match_template2(ctor_match, input);

    if (template_match_result)
    {
        co_return co_await QUX_CO_DEP(builtin_ctor_vm_procedure3, (input));
    }
    else if (match_template2(quxlang::parsers::parse_type_symbol("TT(t1)::.DESTRUCTOR#{ @THIS DESTROY& TT(t1)}"), input))
    {
        auto result =  co_await QUX_CO_DEP(builtin_dtor_vm_procedure3, (input));
        co_return result;
    }

    throw compiler_bug("not implemented or bug");
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_ctor_vm_procedure3)
{
    std::string input_name = quxlang::to_string(input);
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_ctor(input);
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_dtor_vm_procedure2)
{
    throw compiler_bug("removed");
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_dtor_vm_procedure3)
{
    co_vmir_generator<compiler_binder> gen(compiler_binder(c), input);

    co_return co_await gen.co_generate_builtin_dtor(input);
}