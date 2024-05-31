//
// Created by Ryan Nicholl on 4/2/24.
//

#ifndef RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/vm_executable_unit.hpp"
#include "quxlang/data/vm_expression.hpp"
#include "quxlang/vmir2/vmir2.hpp"
#include <quxlang/macros.hpp>
namespace quxlang
{

    // This class is meant to replace the expression generator in vm_procedure_from_canonical_functanoid_resolver,
    // The goal is to generalize the expression emitter that can produce QXVM IR in non-function contexts as
    class co_vmir_expression_emitter
    {
        using storage_index = std::size_t;
        using deferral_index = std::size_t;

      public:
        class co_interface
        {
          public:
            virtual QUX_SUBCO_MEMBER_FUNC(emit, void, (quxlang::vmir2::vm_instruction)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, storage_index, (type_symbol type)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< storage_index >, (type_symbol sym)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(index_type, type_symbol, (storage_index index)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(create_string_literal, storage_index, (std::string str)) = 0;
            virtual QUX_SUBCO_MEMBER_FUNC(create_numeric_literal, storage_index, (std::string str)) = 0;

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

        QUX_SUBCO_MEMBER_FUNC(generate_expr, vmir2::storage_index, (expression input));
        QUX_SUBCO_MEMBER_FUNC(typeof_vm_value, type_symbol, (expression input));

      private:
        QUX_SUBCO_MEMBER_FUNC(emit_invoke, storage_index, (type_symbol what, vmir2::invocation_args input));

        QUX_SUBCO_MEMBER_FUNC(gen_call_expr, storage_index, (expression_call call));

        QUX_SUBCO_MEMBER_FUNC(gen_call_functum, storage_index, (type_symbol what, vmir2::invocation_args input));
        QUX_SUBCO_MEMBER_FUNC(gen_call_functanoid, void, (instanciation_reference what, vmir2::invocation_args input));
        QUX_SUBCO_MEMBER_FUNC(gen_invoke, void, (instanciation_reference what, vmir2::invocation_args input));

        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_symbol_reference sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_binary sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_dotreference sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_thisdot_reference sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_numeric_literal sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_call sym));
        QUX_SUBCO_MEMBER_FUNC(generate, storage_index, (expression_this_reference sym));

        QUX_SUBCO_MEMBER_FUNC(generate_field_access, storage_index , (storage_index what, std::string field_name));
    };
} // namespace quxlang

#endif // RPNX_QUXLANG_CO_VMIR_EXPRESSION_EMITTER_HEADER
