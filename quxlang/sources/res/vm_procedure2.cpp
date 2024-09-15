//
// Created by Ryan Nicholl on 5/11/24.
//

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/res/expr/co_vmir_routine_emitter.hpp"
#include <deque>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/res/vm_procedure2.hpp>

namespace quxlang
{

  
} // namespace quxlang

QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    vm_procedure2_generator gen(compiler_binder(c), input);
  throw rpnx::unimplemented();
   // co_return co_await gen.generate();
}