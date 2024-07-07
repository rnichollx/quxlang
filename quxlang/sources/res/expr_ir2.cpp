//
// Created by Ryan Nicholl on 7/6/24.
//
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/compiler_binding.hpp>
#include <quxlang/res/expr_ir2.hpp>

namespace quxlang::impl
{
    class expr_ir2_binder : public compiler_binder
    {
        type_symbol m_context;
        expression m_expr;
        vmir2::functanoid_routine m_result;

        std::vector<bool> m_slot_alive;

      public:
        expr_ir2_binder(compiler* c, type_symbol context, expression expr) : compiler_binder(c), m_context(context), m_expr(expr)
        {
        }

        auto& get()
        {
            return m_result;
        }

        auto lookup_symbol(type_symbol sym) -> co_type< std::optional< vmir2::storage_index > >
        {
            auto canonical_symbol = co_await canonical_symbol_from_contextual_symbol(contextual_type_reference{.context = m_context, .type = sym});

            std::string symbol_str = to_string(canonical_symbol);
            // TODO: Check if global variable
            bool is_global_variable = false;
            bool is_function = true; // This might not actually be true

            vm_expr_bound_value result;
            result.function_ref = canonical_symbol;
            result.value = void_value{};

            throw rpnx::unimplemented();

            co_return 0;
        }

        auto index_type(vmir2::storage_index index) -> co_type< type_symbol >
        {
            if (index < m_result.slots.size())
            {
                co_return m_result.slots.at(index).type;
            }
            else
            {
                throw std::make_exception_ptr(std::logic_error("Not found"));
            }
        }

        auto slot_alive(vmir2::storage_index index) -> co_type< bool >
        {
            if (index < m_result.slots.size())
            {
                co_return m_slot_alive.at(index);
            }
            else
            {
                throw std::make_exception_ptr(std::logic_error("Not found"));
            }
        }

        auto create_temporary_storage(type_symbol type) -> co_type< vmir2::storage_index >
        {
            vmir2::storage_index index = m_result.slots.size();
            m_result.slots.push_back(vmir2::vm_slot{.type = type, .kind = vmir2::slot_kind::local});
            m_slot_alive.push_back(true);
            co_return index;
        }
    };
} // namespace quxlang::impl

QUX_CO_RESOLVER_IMPL_FUNC_DEF(expr_ir2)
{

    quxlang::impl::expr_ir2_binder binder(c, input.context, input.expr);
    co_vmir_expression_emitter< quxlang::impl::expr_ir2_binder > emitter(binder);
    auto result = co_await emitter.generate_expr(input.expr);

    co_return binder.get();
}
