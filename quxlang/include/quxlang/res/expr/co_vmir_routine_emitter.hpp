// Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com
#ifndef QUXLANG_RES_EXPR_CO_VMIR_ROUTINE_EMITTER_HEADER_GUARD
#define QUXLANG_RES_EXPR_CO_VMIR_ROUTINE_EMITTER_HEADER_GUARD

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
        vm_procedure2_generator(CoroutineProvider arg_prv, instanciation_reference func_arg) : prv(arg_prv), func(func_arg)
        {
        }
        using block_index_t = std::size_t;

      private:
        instanciation_reference func;
        CoroutineProvider prv;
        vmir2::frame_generation_state frame;

        using co_slot = typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;
        using co_block = typename CoroutineProvider::template co_type< quxlang::vmir2::block_index >;
        using co_void = typename CoroutineProvider::template co_type< void >;

        auto generate_arg_slots() -> typename CoroutineProvider::template co_type< void >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(func));
            // Precondition: Func is a fully instanciated symbol
            instanciation_reference const& inst = func;

            auto sig = co_await prv.functanoid_sigtype(inst);

            type_symbol return_type = sig.return_type.value_or(void_type()); // co_await prv.functanoid_return_type(inst);

            if (!typeis< void_type >(return_type))
            {
                type_symbol return_parameter_type = create_nslot(return_type);
                frame.entry_block().create_named_argument("RETURN", return_parameter_type, std::nullopt);
            }

            auto arg_names = co_await prv.function_param_names(inst.temploid);

            std::size_t positional_index = 0;
            for (auto const& param_name : arg_names.positional)
            {
                type_symbol const& param_type = inst.params.positional.at(positional_index);
                frame.entry_block().create_positional_argument(param_type, param_name);
            }
            for (auto const& [api_name, param_type] : inst.params.named)
            {
                std::optional< std::string > arg_name;
                if (arg_names.named.contains(api_name))
                {
                    arg_name = arg_names.named.at(api_name);
                }

                frame.entry_block().create_named_argument(api_name, param_type, arg_name);
            }

            vmir2::state_engine::apply_entry(frame.entry_block().block.entry_state, frame.slots.slots);

            co_return;
        }

      public:
        auto generate() -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine2 >
        {
            frame.generate_entry_block();
            co_await generate_arg_slots();
            if (!co_await prv.function_builtin(func.temploid))
            {
                co_await generate_body();
                if (typeis< submember >(func.temploid.templexoid) && func.temploid.templexoid.template get_as< submember >().name == "CONSTRUCTOR")
                {
                    co_await generate_ctor_delegates();
                }
            }
            co_await generate_dtors();
            co_return frame.get_result();
        }

        auto generate_builtin_ctor() -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine2 >
        {
            frame.generate_entry_block();
            co_await generate_arg_slots();

            // co_await generate_body();
            co_await generate_ctor_delegates({});
            co_return frame.get_result();
        }

        auto generate_builtin_dtor() -> typename CoroutineProvider::template co_type< quxlang::vmir2::functanoid_routine2 >
        {
            frame.generate_entry_block();
            co_await generate_arg_slots();
            co_await generate_body();
            co_await generate_dtors();
            co_return frame.get_result();
        }

      private:
        [[nodiscard]] auto generate_expression(block_index_t& current_block, expression const& expr) -> typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >
        {
            using V = typename CoroutineProvider::template co_type< quxlang::vmir2::storage_index >;
            co_vmir_expression_emitter emitter(prv, func, frame.block(current_block));
            co_return co_await emitter.generate_expr(expr);
        }

        [[nodiscard]] auto generate_bool_expr(block_index_t current_block, expression const& expr) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
        {
            auto expr_index = co_await generate_expression(current_block, expr);
            // TODO: Convert to bool if the result is not type bool
            co_return expr_index;
        }

        [[nodiscard]] auto generate_void_expr(block_index_t current_block, expression const& expr) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(expr));
            co_await generate_expression(current_block, expr);
            co_return 0;
        }

        [[nodiscard]] auto generate_ctor_delegates() -> typename CoroutineProvider::template co_type< void >
        {
            temploid_reference const& function = func.temploid;

            std::optional< ast2_function_declaration > const& function_ast = co_await prv.function_declaration(function);

            QUXLANG_COMPILER_BUG_IF(!function_ast.has_value(), "Expected function declaration to be defined");

            auto const& decl = function_ast.value();

            std::vector< delegate > delegates;

            for (auto& dlg : decl.definition.delegates)
            {
                // TODO: support complex types

                if (!dlg.target.type_is< submember >() || !dlg.target.get_as< submember >().of.type_is< context_reference >())
                {
                    throw rpnx::unimplemented();
                }

                delegate dlg2;
                dlg2.name = dlg.target.get_as< submember >().name;

                // TODO: Support named arguments in delegates
                for (expression const& arg : dlg.args)
                {
                    dlg2.args.push_back(expression_arg{.value = arg});
                }

                delegates.push_back(dlg2);
            }

            co_await generate_ctor_delegates(delegates);

            co_return;
        }

        [[nodiscard]] auto generate_ctor_delegates(std::vector< delegate > delegates) -> typename CoroutineProvider::template co_type< void >
        {
            std::size_t current_block = frame.entry_block_id();

            instanciation_reference const& inst = func;
            auto const& sel = inst.temploid;

            auto functum = sel.templexoid;

            type_symbol cls;

            QUXLANG_COMPILER_BUG_IF(!typeis< submember >(functum), "Expected constructor to be submember");

            cls = as< submember >(functum).of;

            // This function is for default ctors, it should just default construct all member variables.

            auto const& fields = co_await prv.class_field_list(cls);

            vmir2::invocation_args fields_args;

            for (class_field const& fld : fields)
            {
                auto fslot = frame.slots.create_temporary(fld.type);
                fields_args.named[fld.name] = fslot;
            }

            auto thisidx = frame.lookup(current_block, "THIS");

            QUXLANG_COMPILER_BUG_IF(!thisidx.has_value(), "Expected THIS to be defined");

            auto thisidx_value = thisidx.value();

            frame.block(current_block).emit(vmir2::struct_delegate_new{.on_value = thisidx_value, .fields = fields_args});

            std::set< std::string > found_delegate_names;
            for (delegate const& dlg : delegates)
            {
                found_delegate_names.insert(dlg.name);
            }

            // TODO: Drop temporaries between loop iterations
            for (class_field const& fld : fields)
            {
                if (!found_delegate_names.contains(fld.name))
                {
                    auto ctor = submember{.of = fld.type, .name = "CONSTRUCTOR"};
                    vmir2::invocation_args args;
                    args.named["THIS"] = fields_args.named.at(fld.name);

                    co_vmir_expression_emitter< CoroutineProvider > emitter(prv, func, frame.block(current_block));
                    co_await emitter.gen_call_functum(ctor, args);
                }
            }

            //frame.block(current_block).emit(vmir2::struct_complete_new{.on_value = thisidx_value});


        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_if_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            block_index_t after_block = frame.generate_subblock(current_block, "if_statement_after");
            block_index_t condition_block = frame.generate_subblock(current_block, "if_statement_condition");
            block_index_t if_block = frame.generate_subblock(current_block, "if_block");

            frame.generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            frame.generate_branch(cond, condition_block, if_block, after_block);

            co_await generate_function_block(if_block, st.then_block, "if_then");
            frame.generate_jump(if_block, after_block);

            if (st.else_block.has_value())
            {
                block_index_t else_block = frame.generate_subblock(current_block, "if_statement_else");
                co_await generate_function_block(else_block, *st.else_block, "if_else");
                frame.generate_jump(else_block, after_block);
            }
            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_while_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            block_index_t condition_block = frame.generate_subblock(current_block, "while_condition");
            block_index_t body_block = frame.generate_subblock(current_block, "while_body");
            block_index_t after_block = frame.generate_subblock(current_block, "while_after");

            frame.generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            frame.generate_branch(condition_block, cond, body_block, after_block);
            co_await generate_function_block(body_block, st.loop_block, "while_statement");
            frame.generate_jump(body_block, condition_block);

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_expression_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            QUXLANG_DEBUG_VALUE(quxlang::to_string(st.expr));

            QUXLANG_COMPILER_BUG_IF(frame.has_terminator(current_block), "Expected no terminator in current block");

            block_index_t expr_block = frame.generate_subblock(current_block, "expr_statement");
            block_index_t after_block = frame.generate_subblock(current_block, "expr_after");

            frame.generate_jump(current_block, expr_block);
            co_await generate_void_expr(expr_block, st.expr);
            frame.generate_jump(expr_block, after_block);

            current_block = after_block;

            co_return;
        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_block const& st) -> typename CoroutineProvider::template co_type< void >
        {
            co_await generate_function_block(current_block, st, "function_block");
            co_return;
        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_var_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            std::string type_str = quxlang::to_string(st.type);
            std::string context_str = quxlang::to_string(func);
            type_symbol var_type = (co_await prv.lookup({.context = func, .type = st.type})).value();

            auto idx = co_await generate_variable(current_block, st.name, var_type);

            std::string var_type_name = quxlang::to_string(var_type);

            vmir2::invocation_args args;

            args.named["THIS"] = idx;

            // Generate new blocks for after an intialization steps.
            auto new_block = frame.generate_subblock(current_block, "var_new");
            auto after_block = frame.generate_subblock(current_block, "var_after");
            frame.generate_jump(current_block, new_block);
            current_block = new_block;

            // TODO: Function var statement needs named constructor support
            for (auto const& init : st.initializers)
            {
                auto init_idx = co_await generate_expression(current_block, init);
                args.positional.push_back(init_idx);
            }

            co_vmir_expression_emitter< CoroutineProvider > emitter(prv, func, frame.block(current_block));

            auto ctor = submember{.of = var_type, .name = "CONSTRUCTOR"};
            co_await emitter.gen_call_functum(ctor, args);

            auto class_default_dtor = co_await prv.class_default_dtor(var_type);
            if (class_default_dtor)
            {
                if (!frame.non_trivial_dtors.contains(var_type))
                {
                    frame.non_trivial_dtors[var_type] = class_default_dtor.value();
                }

                // TODO: Consider re-adding this for non-default dtors later.
                // co_await emitter.gen_defer_dtor(idx, dtor.value(), vmir2::invocation_args{.named = {{"THIS", idx}}});
            }

            vmir2::slot_state new_state = frame.block(current_block).current_slot_states[idx];

            frame.generate_jump(current_block, after_block);
            current_block = after_block;

            // the after_block is cloned from the parent block, so the new variable isn't alive in that block
            // We want all the temporaries to be destroyed so we cloned the parent block twice, and the after
            // block is the parent + the new variable, which won't contain the temporary objects generated above.
            frame.block(current_block).current_slot_states[idx] = new_state;
            frame.block(current_block).block.entry_state[idx] = new_state;
            frame.block(current_block).named_references[st.name] = idx;

            co_return;
        }

        [[nodiscard]] auto generate_variable(block_index_t& current_block, std::string const& name, type_symbol typ) -> typename CoroutineProvider::template co_type< vmir2::storage_index >
        {
            auto idx = frame.entry_block().create_variable(typ, name);

            auto dtor = co_await prv.class_default_dtor(typ);
            if (dtor.has_value())
            {
                if (!frame.non_trivial_dtors.contains(typ))
                {
                    frame.non_trivial_dtors[typ] = *dtor;
                }
            }
            co_return idx;
        }

        [[nodiscard]] auto generate_statement_ovl(block_index_t& current_block, function_return_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            auto return_arg_opt = frame.lookup(current_block, "RETURN");

            if (return_arg_opt.has_value())
            {
                auto return_arg = return_arg_opt.value();

                if (st.expr.has_value())
                {
                    auto expr_index = co_await generate_expression(current_block, st.expr.value());
                    vmir2::invocation_args args;
                    args.named["THIS"] = return_arg;
                    args.named["OTHER"] = expr_index;

                    co_vmir_expression_emitter< CoroutineProvider > emitter(prv, func, frame.block(current_block));
                    auto return_type = frame.block(current_block).current_type(return_arg);
                    if (!typeis< nvalue_slot >(return_type))
                    {
                        throw std::logic_error("RETURN parameter has the wrong type");
                    }
                    return_type = type_symbol(as< nvalue_slot >(return_type).target);
                    auto ctor = submember{.of = return_type, .name = "CONSTRUCTOR"};
                    co_await emitter.gen_call_functum(ctor, args);
                }

                frame.generate_return(current_block);
            }
            else
            {
                auto return_type = co_await prv.functanoid_return_type(func);
                assert(typeis< void_type >(return_type));
                frame.generate_return(current_block);
            }

            co_return;
        }

        [[nodiscard]] auto generate_fblock_statement(block_index_t& current_block, function_statement const& st) -> typename CoroutineProvider::template co_type< void >
        {
            co_await rpnx::apply_visitor< typename CoroutineProvider::template co_type< void > >(
                [&](auto st) -> typename CoroutineProvider::template co_type< void >
                {
                    co_return co_await this->generate_statement_ovl(current_block, st);
                },
                st);
            co_return;
        }

        [[nodiscard]] auto generate_function_block(block_index_t& current_block, function_block const& block, std::string block_from) -> typename CoroutineProvider::template co_type< void >
        {
            assert(!frame.has_terminator(current_block));
            auto new_block = frame.generate_subblock(current_block, block_from + "_block_new");

            assert(!frame.has_terminator(new_block));

            auto after_block = frame.generate_subblock(current_block, block_from + "_block_after");
            assert(!frame.has_terminator(after_block));

            frame.generate_jump(current_block, new_block);

            //

            for (auto const& statement : block.statements)
            {
                assert(!frame.has_terminator(new_block));
                co_await generate_fblock_statement(new_block, statement);
                // assert(!frame.has_terminator(new_block));
            }

            if (!frame.has_terminator((new_block)))
            {
                frame.generate_jump(new_block, after_block);
            }

            assert(frame.has_terminator(current_block));
            current_block = after_block;
            assert(!frame.has_terminator(after_block));
            co_return;
        }

        auto generate_dtors() -> typename CoroutineProvider::template co_type< void >
        {
            // Loop through all local slots and check if they have non-trivial dtors, then add
            // dtor references to non_trivial_dtors if they do.
            for (auto const& slot : frame.slots.slots)
            {
                auto dtor = co_await prv.class_default_dtor(slot.type);
                if (dtor.has_value())
                {
                    assert(!frame.non_trivial_dtors.contains(slot.type) || frame.non_trivial_dtors[slot.type] == *dtor);
                    frame.non_trivial_dtors[slot.type] = *dtor;
                }
            }

            co_return;
        }

        auto generate_body() -> typename CoroutineProvider::template co_type< void >
        {
            auto const& inst = func;

            auto& function_ref = inst.temploid;

            auto function_decl_opt = co_await this->prv.function_declaration(function_ref);
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            std::size_t block = frame.entry_block_id();
            co_await generate_function_block(block, function_decl.definition.body, "body");

            // TODO: Implement falloff
        }
    };
} // namespace quxlang

#endif