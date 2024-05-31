//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER
#define RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER

#include "quxlang/data/expression.hpp"
#include "quxlang/data/interp_value.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include "quxlang/vmir2/vmir2.hpp"

namespace quxlang
{

    class co_interpreter
    {
      private:
        compiler* c;

        class co_expr_interface : public virtual co_vmir_expression_emitter::co_interface
        {
            co_interpreter* ci;

          public:
            co_expr_interface(co_interpreter* ci)
                : ci(ci)
            {
            }

            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, vmir2::storage_index, (type_symbol type)) override;

            QUX_SUBCO_MEMBER_FUNC(emit, void, (vmir2::vm_instruction what)) override;
        };

      public:
        co_interpreter(compiler* c)
            : c(c)
        {
        }

        QUX_SUBCO_MEMBER_FUNC(pointer_add, interp_value, (interp_value ptr, interp_value offset));
        QUX_SUBCO_MEMBER_FUNC(exec_call, interp_value, (interp_value callee, std::vector< interp_value > args));
        QUX_SUBCO_MEMBER_FUNC(allocate, std::size_t, (type_symbol type, std::size_t count));
        QUX_SUBCO_MEMBER_FUNC(eval, interp_value, (expr_interp_input i_input));
    };

    QUX_CO_RESOLVER(interpret_value, expr_interp_input, interp_value);
} // namespace quxlang

#endif // RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER
