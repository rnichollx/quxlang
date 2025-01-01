// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/res/expr/co_vmir_routine_emitter.hpp"
#include <quxlang/res/vm_procedure2.hpp>
#include <quxlang/compiler.hpp>

#include <quxlang/parsers/parse_type_symbol.hpp>


QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    if (!input.template type_is< initialization_reference >())
    {
        throw compiler_bug("this shouldn't be possible to call");
    }

    initialization_reference const& inst = as< initialization_reference >(input);

    temploid_reference sel = inst.initializee.get_as< temploid_reference >();

    if (sel.which.builtin)
    {
        co_return co_await QUX_CO_DEP(builtin_vm_procedure2, (input));
    }
    else
    {
        co_return co_await QUX_CO_DEP(user_vm_procedure2, (input));
    }
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(user_vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_ctor_vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate_builtin_ctor();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_vm_procedure2)
{
    auto type_match = quxlang::parsers::parse_type_symbol("T(t1)::.CONSTRUCTOR#{BUILTIN; @THIS NEW& T(t1)}");


    QUXLANG_DEBUG_NAMED_VALUE(type_match_str, quxlang::to_string(type_match));

    auto template_match_result = match_template2(type_match, input);

    if (template_match_result)
    {
        co_return co_await QUX_CO_DEP(builtin_ctor_vm_procedure2, (input));
    }
    else if (match_template2(quxlang::parsers::parse_type_symbol("T(t1)::.DESTRUCTOR#{BUILTIN; @THIS DESTROY& T(t1)}"), input))
    {
        auto result =  co_await QUX_CO_DEP(builtin_dtor_vm_procedure2, (input));
        co_return result;
    }

    throw compiler_bug("not implemented or bug");
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_dtor_vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate_builtin_dtor();
}