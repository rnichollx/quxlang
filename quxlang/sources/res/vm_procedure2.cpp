//
// Created by Ryan Nicholl on 5/11/24.
//
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/res/vm_procedure2.hpp>

namespace quxlang
{
    enum class slot_kind {
        argument,
        variable,
        temporary,
    };

    enum class slot_state {
        alive,
        dead,
        call_pending,
    };

    class vm_procedure2_generator_state
    {
        type_symbol func;

        struct slot_static_information
        {
            slot_kind kind;
            type_symbol type;
        };

        struct slot_dynamic_information
        {
            bool alive = false;
        };

        class expr_interface
        {
            vm_procedure2_generator_state* gen;

          public:
        };

        std::vector< slot_static_information > slot_info;
        std::vector< slot_dynamic_information > slot_states;

      public:
        vm_procedure2_generator_state(type_symbol what)
            : func(what)
            , ifc(this)
        {
        }

      public:
        class interface : public co_vmir_expression_emitter::co_interface
        {
            vm_procedure2_generator_state* gen;

          public:
            interface(vm_procedure2_generator_state* gen)
                : gen(gen)
            {
            }

            virtual QUX_SUBCO_MEMBER_FUNC(create_temporary_storage, vmir2::storage_index, (type_symbol type)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(lookup_symbol, std::optional< vmir2::storage_index >, (type_symbol sym)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(emit, void, (vmir2::vm_instruction)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(index_type, type_symbol, (vmir2::storage_index)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(create_string_literal, vmir2::storage_index, (std::string)) override;
            virtual QUX_SUBCO_MEMBER_FUNC(create_numeric_literal, vmir2::storage_index, (std::string)) override;
        };

      private:
        interface ifc;

      public:
        interface* get_interface()
        {
            return &ifc;
        }

        void print()
        {
        }
    };
} // namespace quxlang

QUX_SUBCO_MEMBER_FUNC_DEF2(quxlang::vm_procedure2_generator_state, interface, create_temporary_storage, quxlang::vmir2::storage_index, (type_symbol type))
{
    auto index = this->gen->slot_info.size();
    this->gen->slot_info.push_back({.type = type, .kind = slot_kind::temporary});
    co_return index;
}



QUX_SUBCO_MEMBER_FUNC_DEF2(quxlang::vm_procedure2_generator_state, interface, lookup_symbol, std::optional< quxlang::vmir2::storage_index >, (type_symbol sym))
{
    throw rpnx::unimplemented();
}


QUX_SUBCO_MEMBER_FUNC_DEF2(quxlang::vm_procedure2_generator_state, interface, emit, void, (vmir2::vm_instruction instr))

{
    throw rpnx::unimplemented();
}


QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    vm_procedure2_generator_state gen(input);

    co_vmir_expression_emitter expr_emitter(c, gen.get_interface());

    expression expr = parsers::parse_expression("foo(I64:(1), I64:(2))");

    expr_emitter.generate_expr(expr);

    gen.print();

    throw rpnx::unimplemented();
}