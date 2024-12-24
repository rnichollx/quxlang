// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/res/expr/co_vmir_routine_emitter.hpp"
#include <quxlang/res/vm_procedure2.hpp>
#include <quxlang/compiler.hpp>


QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{


    if (!input.template type_is< instantiation_type >())
    {
        throw compiler_bug("this shouldn't be possible to call");
    }

    instantiation_type const& inst = as< instantiation_type >(input);

    selection_reference sel = inst.callee.get_as< selection_reference >();

    if (sel.overload.builtin)
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
    throw rpnx::unimplemented();
}

QUX_CO_RESOLVER_IMPL_FUNC_DEF(builtin_dtor_vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);

    co_return co_await gen.generate_builtin_dtor();
}