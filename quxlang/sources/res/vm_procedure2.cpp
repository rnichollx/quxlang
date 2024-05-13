//
// Created by Ryan Nicholl on 5/11/24.
//
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
            slot_state state;

            type_symbol dtor;
            std::optional<vm_callargs> dtor_args;
        };

        struct slot_state
        {
            bool alive = false;
        };

        class expr_interface
        {
            vm_procedure2_generator_state * gen;
          public:
        };

      public:
        vm_procedure2_generator_state(type_symbol what)
            : func(what)
        {
        }
    };
} // namespace quxlang

QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    vm_procedure2_generator_state gen(input);
}