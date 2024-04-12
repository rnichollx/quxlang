//
// Created by Ryan Nicholl on 4/2/24.
//

#ifndef RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/vm_expression.hpp"
namespace quxlang
{

    // This class is meant to replace the expression generator in vm_procedure_from_canonical_functanoid_resolver,
    // The goal is to generalize the expression emitter that can produce QXVM IR in non-function contexts as
    class co_vmir_expression_emitter
    {
      public:
        class co_interface
        {
          public:
            using storage_index = std::size_t;

            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary, storage_index, (vm_type type, std::vector< vm_value > ctor_params)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< vm_value >, (expression_symbol_reference sym)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(invoke_functanoid, vm_value, (type_symbol function, std::vector< vm_value > params)) = 0;
        };

      private:
        compiler* c;
        co_interface* inter;

      public:
        co_vmir_expression_emitter(compiler* c, co_interface* inter)
            : c(c)
            , inter(inter)
        {
        }

        QUX_SUBCO_MEMBER_FUNC(emit_vm_value, vm_value, (expression input));

      private:
        QUX_SUBCO_MEMBER_FUNC(emit_invoke, vm_value, (type_symbol what, std::vector< vm_value > input));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_symbol_reference sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_binary sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_dotreference sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_thisdot_reference sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_numeric_literal sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_call sym));
        QUX_SUBCO_MEMBER_FUNC(emit_value, vm_value, (expression_this_reference sym));
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
