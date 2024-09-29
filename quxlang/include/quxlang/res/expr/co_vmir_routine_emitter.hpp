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
        type_symbol func;
        CoroutineProvider prv;
        vmir2::frame_generation_state frame;

        using co_slot = CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;
        using co_block = CoroutineProvider::template co_type< quxlang::vmir2::block_index >;
        using co_void = CoroutineProvider::template co_type< void >;












        auto generate_arg_slots() -> CoroutineProvider::template co_type< void >
        {


            // Precondition: Func is a fully instanciated symbol
            instanciation_reference const& inst = as< instanciation_reference >(func);

            std::optional< type_symbol > return_type = co_await prv.functanoid_return_type(inst);

            if (return_type)
            {
                frame.entry_block().create_named_argument("RETURN", *return_type, std::nullopt);
            }

            ast2_function_declaration decl = co_await prv.function_declaration(inst);

            std::size_t positional_index = 0;
            for (auto const& param : decl.header.call_parameters )
            {
                if (param.api_name.has_value())
                {
                    type_symbol arg = inst.parameters.named_parameters.at(*param.api_name);

                    frame.entry_block().create_named_argument(*param.api_name, arg, param.name);
                }
                else
                {
                    type_symbol arg = inst.parameters.positional_parameters.at(positional_index);
                    positional_index++;
                    frame.entry_block().create_positional_argument(arg, param.name);
                }
            }

            co_return;
        }

      public:
        auto generate() -> CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine2 >
        {
            frame.generate_entry_block();
            co_await generate_arg_slots();
            co_await generate_body();
            co_return frame.get_result();;
        }

      private:
        auto generate_expression_ovl(block_index_t& current_block, expression_this_reference expr) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {

            auto this_index_opt = frame.block(current_block).lookup("THIS");
            if (!this_index_opt.has_value())
            {
                throw std::logic_error("THIS not found in this context");
            }
            auto this_index = this_index_opt.value();
            auto ref_index = co_await generate_ref(current_block, this_index);
        }

        auto generate_expression(block_index_t& current_block, expression const& expr) -> CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            using V = CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;
            co_vmir_expression_emitter emitter(prv, func, frame.block(current_block));
            co_return co_await emitter.generate_expr(expr);
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
            block_index_t after_block =  frame.generate_subblock(current_block);
            block_index_t condition_block = frame.generate_subblock(current_block);
            block_index_t if_block = frame.generate_subblock(current_block);

            frame.generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            frame.generate_branch(condition_block, cond, if_block, after_block);

            co_await generate_function_block(if_block, st.then_block);
            frame.generate_jump(if_block, after_block);

            if (st.else_block.has_value())
            {
                block_index_t else_block = frame.generate_subblock(current_block);
                co_await generate_function_block(else_block, *st.else_block);
                frame.generate_jump(else_block, after_block);
            }
            current_block = after_block;

            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_while_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            block_index_t condition_block = frame.generate_subblock(current_block);
            block_index_t body_block = frame.generate_subblock(current_block);
            block_index_t after_block = frame.generate_subblock(current_block);

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
            block_index_t expr_block = frame.generate_subblock(current_block);
            block_index_t after_block = frame.generate_subblock(current_block);

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
            throw rpnx::unimplemented();
            co_return;
        }

        auto generate_statement_ovl(block_index_t& current_block, function_return_statement const& st) -> CoroutineProvider::template co_type< void >
        {
            throw rpnx::unimplemented();
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
            auto new_block = frame.generate_subblock(current_block);
            auto after_block = frame.generate_subblock(current_block);
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

            co_await generate_function_block(frame.entry_block_id(), function_decl.definition.body);
        }


    };
} // namespace quxlang

#endif