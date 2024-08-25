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

        std::vector< bool > m_slot_alive;

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

            auto kind = co_await this->symbol_type(canonical_symbol);

            vmir2::storage_index index = 0;


            auto binding = co_await create_binding(0, canonical_symbol);



            if (kind == quxlang::symbol_kind::global_variable)
            {
                auto variable_type = co_await this->variable_type(canonical_symbol);
                index = co_await create_reference_internal(binding, variable_type);
            }

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

        auto create_binding(vmir2::storage_index slot, type_symbol type) -> co_type< vmir2::storage_index >
        {
            vmir2::storage_index index = m_result.slots.size();
            m_result.slots.push_back(vmir2::vm_slot{.type = type,  .binding_of = slot, .kind = vmir2::slot_kind::binding});
            m_slot_alive.push_back(true);
            co_return index;
        }

        auto implicit_cast_reference(vmir2::storage_index index, type_symbol target) -> co_type< vmir2::storage_index >
        {
            if (!is_ref_implicitly_convertible_by_syntax(co_await index_type(index), target))
            {
                throw std::logic_error("Cannot implicitly cast");
            }

            auto ref = co_await create_reference_internal(index, target);

            co_return ref;
        }

        auto emit(vmir2::access_field fld) -> co_type< void >
        {
            m_result.instructions.push_back(fld);
            m_slot_alive.at(fld.store_index) = true;
            co_return;
        }

        co_type< vmir2::storage_index > create_numeric_literal(std::string literal)
        {
            vmir2::storage_index idx = m_result.slots.size();
            m_result.slots.push_back(vmir2::vm_slot{.type = numeric_literal_reference{}, .name = "LITERAL" + std::to_string(idx), .literal_value = literal});
            m_slot_alive.push_back(true);
            co_return idx;
        }

        co_type< vmir2::storage_index > create_reference_internal(vmir2::storage_index index, type_symbol new_type)
        {
            // This function is used to handle the case where we have an index and need to force it into a
            // reference type.
            // This is mainly used in three places, implied ctor "THIS" argument, dtor, and when a symbol
            // is encountered during an expression.
            auto ty = co_await index_type(index);

            std::vector< std::string > types;
            for (auto& slot : m_result.slots)
            {
                types.push_back(quxlang::to_string(slot.type));
            }

            vmir2::storage_index temp = co_await create_temporary_storage(new_type);

            vmir2::make_reference ref;
            ref.value_index = index;
            ref.reference_index = temp;

            // TODO: make this not a hack
            co_await this->emit_instruction(ref);

            std::string slot_type = quxlang::to_string(m_result.slots.at(temp).type);
            // this is kind of a hack, but we can presume this after the make_reference call.
            this->m_slot_alive[temp] = true;

            co_return temp;
        }

        co_type< void > emit_instruction(vmir2::vm_instruction inst)
        {
            m_result.instructions.push_back(inst);
            co_return;
        }

        co_type< void > emit(vmir2::invoke ivk)
        {
            type_symbol invoked_symbol = ivk.what;
            vmir2::invocation_args args = ivk.args;
            std::cout << "emit_invoke(" << quxlang::to_string(invoked_symbol) << ")" << " " << quxlang::to_string(args) << std::endl;

            co_await emit_instruction(ivk);

            instanciation_reference inst = as< instanciation_reference >(invoked_symbol);

            for (auto& arg : args.positional)
            {
                type_symbol arg_type = co_await index_type(arg);
                if (typeis< nvalue_slot >(arg_type))
                {
                    if (m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
                    }
                    m_slot_alive.at(arg) = true;
                }

                else if (typeis< dvalue_slot >(arg_type))
                {
                    if (!m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
                    }
                    m_slot_alive.at(arg) = false;
                }
                else
                {
                    if (!m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
                    }

                    if (!is_ref(arg_type))
                    {
                        // In quxlang calling convention, callee is responsible for destroying the argument.
                        m_slot_alive.at(arg) = false;
                    }

                    // however, references are not destroyed when passed as arguments.
                }
            }
            for (auto& [name, arg] : args.named)
            {
                type_symbol arg_type = co_await index_type(arg);
                if (name == "RETURN")
                {
                    m_slot_alive.at(arg) = true;
                    continue;
                }
                type_symbol parameter_type = inst.parameters.named_parameters.at(name);

                if (typeis< nvalue_slot >(parameter_type))
                {
                    if (m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a NEW& parameter on a live slot.");
                    }
                    std::cout << "Setting slot " << arg << " alive" << std::endl;
                    m_slot_alive.at(arg) = true;
                }

                else if (typeis< dvalue_slot >(parameter_type))
                {
                    if (!m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a DESTROY& parameter on a dead slot.");
                    }
                    m_slot_alive.at(arg) = false;
                }
                else
                {
                    if (!m_slot_alive.at(arg))
                    {
                        throw std::logic_error("Cannot invoke a functanoid with a parameter on a dead slot.");
                    }

                    if (!is_ref(arg_type))
                    {
                        // In quxlang calling convention, callee is responsible for destroying the argument.
                        // however, references are not destroyed when passed as arguments.
                        std::cout << "Setting slot " << arg << " dead" << std::endl;
                        m_slot_alive.at(arg) = false;
                    }
                }
            }

            co_return;
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
