#ifndef RPNX_QUXLANG_CO_VMIR_ROUTINE_EMITTER_HEADER
#define RPNX_QUXLANG_CO_VMIR_ROUTINE_EMITTER_HEADER

#include <cinttypes>
#include <coroutine>
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "quxlang/vmir2/vmir2.hpp"

namespace quxlang
{
    /*
    template < typename T, typename U >
    concept awaitable_for = requires(T t) {
        {
            t.await_ready()
        } -> std::convertible_to< bool >;
        {
            t.await_resume()
        } -> std::convertible_to< U >;
    };

    template < typename T >
    concept vm_procedure2_generator_coroutine_provider = requires(T t) {
        {
            t.functanoid_return_type(std::declval< instanciation_reference >())
        } -> awaitable_for< type_symbol >;
        {
            t.functum_select_function(std::declval< instanciation_reference >())
        } -> awaitable_for< selection_reference >;
        {
            t.function_declaration(std::declval< selection_reference >())
        } -> awaitable_for< ast2_function_declaration >;
    };
    */

    template < typename CoroutineProvider >
    class vm_procedure2_generator
    {
      public:
        vm_procedure2_generator(CoroutineProvider arg_prv, type_symbol func_arg) : prv(arg_prv), func(func_arg)
        {
        }
        using block_index_t = std::size_t;

      private:
        struct block_lookup_table
        {
            std::map< std::string, vmir2::storage_index > locals;
            std::map< block_index_t, bool > entry_lifetimes;
        };

        struct expression_binder : CoroutineProvider
        {
            block_index_t& current_block;
            vm_procedure2_generator* gen;
            expression_binder(vm_procedure2_generator* gen, block_index_t& current_block) : CoroutineProvider(gen->prv), current_block(current_block), gen(gen)
            {
            }

            auto create_numeric_literal(std::string lit) -> CoroutineProvider::template co_type< vmir2::storage_index >
            {
                auto temp = co_await gen->generate_numeric_literal(lit);
                co_return temp;
            }

            auto lookup_symbol(type_symbol symbol) -> CoroutineProvider::template co_type< std::optional< vmir2::storage_index > >
            {
                bool is_possibly_frame_value = typeis< subentity_reference >(symbol) && typeis< context_reference >(as< subentity_reference >(symbol).parent);

                // Frame values are sub-entities of the current context.
                if (is_possibly_frame_value)
                {
                    std::string name = as< subentity_reference >(symbol).subentity_name;
                    auto it = gen->block_lookup_tables.at(current_block).locals.find(name);

                    if (it != gen->block_lookup_tables.at(current_block).locals.end())
                    {
                        co_return it->second;
                    }
                }

                // assert(ctx.current_context() == m_func_name);
                contextual_type_reference ctype{.context = gen->func, .type = symbol};

                auto canonical_symbol = co_await this->canonical_symbol_from_contextual_symbol(ctype);

                std::string symbol_str = to_string(canonical_symbol);

                // TODO: Check if global variable
                bool is_global_variable = false;
                bool is_function = true; // This might not actually be true
                auto binding = co_await gen->generate_free_binding(canonical_symbol);

                co_return binding;
            }

            auto index_type(vmir2::storage_index index) -> CoroutineProvider::template co_type< type_symbol >
            {
                co_return gen->result.slots.at(index).type;
            }

            auto create_temporary_storage(type_symbol type) -> CoroutineProvider::template co_type< vmir2::storage_index >
            {
                auto temp = co_await gen->generate_temporary(type);
                co_return temp;
            }

            auto emit_instruction(vmir2::vm_instruction instr) -> CoroutineProvider::template co_type< void >
            {
                gen->result.blocks.at(current_block).instructions.push_back(instr);
                co_return;
            }

            auto slot_alive(vmir2::storage_index index) -> CoroutineProvider::template co_type< bool >
            {
                co_return gen->current_lifetimes.at(index);
            }
        };

        type_symbol func;

        CoroutineProvider prv;
        std::deque< std::string > states;
        quxlang::vmir2::functanoid_routine2 result;
        std::map< std::string, vmir2::storage_index > arg_map;
        std::size_t temp_index = 0;
        std::size_t next_block_index = 0;
        std::map< std::string, block_index_t > block_map;
        std::vector< block_lookup_table > block_lookup_tables;
        std::map< block_index_t, bool > current_lifetimes;

        using co_slot = CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;
        using co_block = CoroutineProvider::template co_type< quxlang::vmir2::block_index >;
        using co_void = CoroutineProvider::template co_type< void >;

        auto generate_entry_block() -> typename CoroutineProvider::template co_type< block_index_t >
        {
            auto index = next_block_index++;
            std::string block_name = "BLOCK" + std::to_string(next_block_index);
            this->result.blocks.emplace_back();
            this->block_map[block_name] = index;
            co_return index;
        }

        auto generate_block(block_index_t parent) -> CoroutineProvider::template co_type< block_index_t >
        {
            auto index = next_block_index++;

            std::string block_name;
            block_name = "BLOCK" + std::to_string(index);
            this->result.blocks.emplace_back();
            this->block_map[block_name] = index;
            //            this->block_lookup_tables.at(index) = block_lookup_tables.at(parent);

            co_return index;
        }

        auto generate_subblock(block_index_t parent) -> CoroutineProvider::template co_type< block_index_t >
        {
            auto index = next_block_index++;

            std::string block_name;
            block_name = "BLOCK" + std::to_string(index);
            result.blocks.emplace_back();
            block_map[block_name] = index;
            block_lookup_tables.at(index) = block_lookup_tables.at(parent);
            block_lookup_tables.at(index).entry_lifetimes = current_lifetimes;
            co_return index;
        }

        static auto generate_numeric_literal_f(std::vector< quxlang::vmir2::vm_slot >& slots, std::size_t& temp_index, std::string litstr) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            slots.push_back(vmir2::vm_slot{.type = numeric_literal_reference{}, .name = "LITERAL" + std::to_string(temp_index++), .literal_value = litstr, .kind = vmir2::slot_kind::literal});
            co_return slots.size() - 1;
        }

        auto generate_numeric_literal(std::string lit) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            return generate_numeric_literal_f(this->result.slots, this->temp_index, std::move(lit));
        }

        static auto generate_temporary_f(std::vector< quxlang::vmir2::vm_slot >& slots, std::size_t& temp_index, type_symbol type) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            slots.push_back(vmir2::vm_slot{.type = std::move(type), .name = "TEMP" + std::to_string(temp_index++), .kind = vmir2::slot_kind::local});
            co_return slots.size() - 1;
        }

        auto generate_temporary(type_symbol type) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            return generate_temporary_f(this->result.slots, this->temp_index, std::move(type));
        }

        static auto generate_binding_f(std::vector< quxlang::vmir2::vm_slot >& slots, std::size_t& temp_index, type_symbol type) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            slots.push_back(vmir2::vm_slot{.type = std::move(type), .name = "BIND" + std::to_string(temp_index++), .kind = vmir2::slot_kind::symbol});
            co_return slots.size() - 1;
        }

        auto generate_free_binding(type_symbol type) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            return generate_binding_f(this->result.slots, this->temp_index, std::move(type));
        }

        auto generate_arg_slots() -> CoroutineProvider::template co_type< void >
        {
            // Precondition: null slot already present
            assert(result.slots.size() == 1);

            // Precondition: Func is a fully instanciated symbol
            instanciation_reference const& inst = as< instanciation_reference >(func);

            std::optional< type_symbol > return_type = co_await prv.functanoid_return_type(inst);

            if (return_type)
            {
                result.slots.push_back(vmir2::vm_slot{.type = create_nslot(*return_type), .name = "RETURN", .kind = vmir2::slot_kind::arg});
                block_lookup_tables.at(result.entry_block).locals["RETURN"] = result.slots.size() - 1;
            }

            for (auto const& [name, argtype] : inst.parameters.named_parameters)
            {
                this->result.slots.push_back(vmir2::vm_slot{.type = argtype, .name = name, .kind = vmir2::slot_kind::arg});
                auto index = result.slots.size() - 1;
                this->block_lookup_tables.at(result.entry_block).locals[name] = index;
                if (!typeis< nvalue_slot >(argtype))
                {
                    this->block_lookup_tables.at(this->result.entry_block).entry_lifetimes[index] = true;
                }
            }

            co_return;
        }

      public:
        auto generate() -> CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine2 >
        {
            assert(result.slots.size() == 0);
            result.slots.push_back(vmir2::vm_slot{.type = void_type{}, .name = "VOID", .kind = vmir2::slot_kind::literal});

            result.entry_block = co_await generate_entry_block();

            co_await generate_arg_slots();

            co_await generate_body();

            co_return result;
        }

      private:
        auto generate_jump(block_index_t from, block_index_t to) -> CoroutineProvider::template co_type< void >
        {
            for (auto& [name, index] : block_lookup_tables.at(to).locals)
            {
                if (!block_lookup_tables.at(from).locals.contains(name))
                {
                    throw std::runtime_error("Jump between block skips constructor of " + name + ", GOTO statements may not skip past constructors");
                }
            }
            result.blocks.at(from).terminator = vmir2::jump{.target = to};
            co_return;
        }

        auto generate_branch(block_index_t from, vmir2::storage_index cond, block_index_t true_block, block_index_t false_block) -> CoroutineProvider::template co_type< void >
        {
            result.blocks.at(from).terminator = vmir2::branch{.condition = cond, .target_true = true_block, .target_false = false_block};
            co_return;
        }

        auto generate_ref(block_index_t& current_block, vmir2::storage_index index) -> CoroutineProvider::template co_type< vmir2::storage_index >
        {
            auto refof_type = result.slots.at(index).type;

            if (is_ref(refof_type))
            {
                co_return index;
            }

            else
            {
                auto temp = co_await generate_temporary(make_mref(refof_type));
                result.blocks.at(current_block).instructions.push_back(vmir2::make_reference{.value_index = index});
            }
        }

        auto generate_expression_ovl(block_index_t& current_block, expression_this_reference expr) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            auto this_index = block_lookup_tables.at(result.entry_block).locals["THIS"];
            auto ref_index = co_await generate_ref(current_block, this_index);
        }

        auto generate_expression(block_index_t& current_block, expression const& expr) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            using V = CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;

            expression_binder bind(this, current_block);

            co_vmir_expression_emitter< expression_binder > emitter(bind);

            co_return co_await emitter.generate_expr(expr);
        }

        type_symbol typeof_slot(vmir2::storage_index index)
        {
            return result.slots.at(index).type;
        }

        co_slot generate_call(block_index_t current_block, type_symbol functum_type, vmir2::invocation_args args)
        {
            call_type ct;

            for (auto const& [name, index] : args.named)
            {
                ct.named_parameters[name] = typeof_slot(index);
            }

            for (auto index : args.positional)
            {
                ct.positional_parameters.push_back(typeof_slot(index));
            }

            instanciation_reference inst{.callee = functum_type, .parameters = ct};

            auto called_functanoid = co_await prv.instanciation(inst);
            co_return 0;
        }

        auto generate_bool_expr(block_index_t current_block, expression const& expr) -> CoroutineProvider::template co_type< vmir2::storage_index >
        {
            auto expr_index = co_await generate_expression(current_block, expr);
            // TODO: Convert to bool if the result is not type bool
            co_return expr_index;
        }

        auto generate_void_expr(block_index_t current_block, expression const& expr) -> CoroutineProvider::template co_type< vmir2::storage_index >
        {
            generate_expression(current_block, expr);
            co_return 0;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_if_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            block_index_t after_block = co_await generate_block(current_block);
            block_index_t condition_block = co_await generate_block(current_block);
            block_index_t if_block = co_await generate_block(current_block);

            co_await generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            co_await generate_branch(condition_block, cond, if_block, after_block);

            co_await generate_function_block(if_block, st.then_block);
            co_await generate_jump(if_block, after_block);

            if (st.else_block.has_value())
            {
                block_index_t else_block = co_await generate_block(current_block);
                co_await generate_function_block(else_block, *st.else_block);
                co_await generate_jump(else_block, after_block);
            }
            current_block = after_block;

            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_while_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            block_index_t condition_block = co_await generate_block(current_block);
            block_index_t body_block = co_await generate_block(current_block);
            block_index_t after_block = co_await generate_block(current_block);

            co_await generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            co_await generate_branch(condition_block, cond, body_block, after_block);
            co_await generate_function_block(body_block, st.loop_block);
            co_await generate_jump(body_block, condition_block);

            current_block = after_block;

            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_expression_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            block_index_t expr_block = co_await generate_block(current_block);
            block_index_t after_block = co_await generate_block(current_block);

            co_await generate_jump(current_block, expr_block);
            co_await generate_void_expr(expr_block, st.expr);
            co_await generate_jump(expr_block, after_block);

            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_block const& st) -> CoroutineProvider::template co_type< void >
        {
            co_await generate_function_block(current_block, st);
            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_var_statement const& st) -> CoroutineProvider::template co_type< void >
        {

            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_return_statement const& st) -> CoroutineProvider::template co_type< void >
        {

            co_return;
        }

        auto generate_fblock_statement(block_index_t& current_block, function_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            co_await rpnx::apply_visitor< CoroutineProvider::template co_type< void > >(
                [&](auto st) -> CoroutineProvider::template co_type< void >
                {
                    return this->generate_statement_ovl(current_block, st);
                },
                st);
            co_return;
        }

        auto generate_function_block(block_index_t& current_block, function_block const& block) -> CoroutineProvider::template co_type< void >
        {
            auto new_block = co_await generate_block(current_block);
            auto after_block = co_await generate_block(current_block);
            co_await generate_jump(current_block, new_block);
            co_await generate_jump(new_block, after_block);

            for (auto const& statement : block.statements)
            {
                co_await generate_fblock_statement(new_block, statement);
            }

            current_block = after_block;

            co_return;
        }

        auto generate_body() -> CoroutineProvider::template co_type< void >
        {
            auto inst = as< instanciation_reference >(func);

            auto function_ref_opt = co_await this->prv.functum_select_function(inst);
            assert(function_ref_opt.has_value());
            auto& function_ref = function_ref_opt.value();

            auto function_decl_opt = co_await this->prv.function_declaration(function_ref);
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            block_index_t block = result.entry_block;
            co_await generate_function_block(block, function_decl.definition.body);
        }

        auto generate_return_block() -> CoroutineProvider::template co_type< void >
        {
            block_index_t return_block = co_await generate_entry_block();
            result.return_block = return_block;
        }
    };
} // namespace quxlang

#endif