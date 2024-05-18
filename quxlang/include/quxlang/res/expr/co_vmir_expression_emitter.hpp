//
// Created by Ryan Nicholl on 4/2/24.
//

#ifndef RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/vm_expression.hpp"
#include <quxlang/macros.hpp>
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
            using deferral_index = std::size_t;

            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, storage_index, (type_symbol type)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(get_temporary_storage_ref, storage_index, (type_symbol type)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_reference, storage_index, (type_symbol type, vm_value init)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(defer_always, deferral_index, (type_symbol what, vm_invocation_args with)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(defer_exception, deferral_index, (type_symbol what, vm_invocation_args with)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(defer_noexception, deferral_index, (type_symbol what, vm_invocation_args with)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(cancel_deferral, void, (deferral_index)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< vm_value >, (type_symbol sym)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(mark_invoked, vm_value, (type_symbol function)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(emit, void, (vm_executable_unit)) = 0;
            //virtual QUX_SUBCO_MEMBER_FUNC(set_lifetime, void, (storage_index, bool)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(set_expression_result, void, (vm_value)) = 0;
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
        QUX_SUBCO_MEMBER_FUNC(typeof_vm_value, vm_type, (expression input));

      private:
        QUX_SUBCO_MEMBER_FUNC(emit_invoke, vm_value, (type_symbol what, std::vector< vm_value > input));

        QUX_SUBCO_MEMBER_FUNC(gen_call_expr, vm_value, (expression_call call));

        QUX_SUBCO_MEMBER_FUNC(gen_call_functum, vm_value, (type_symbol what, vm_callargs input));
        QUX_SUBCO_MEMBER_FUNC(gen_call_functanoid, vm_value, (instanciation_reference what, vm_callargs input));
        QUX_SUBCO_MEMBER_FUNC(gen_invoke_functanoid, vm_value, (instanciation_reference what, vm_invocation_args input));

        QUX_SUBCO_MEMBER_FUNC(gen_invoke, vm_value, (type_symbol what, vm_callargs input));
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
