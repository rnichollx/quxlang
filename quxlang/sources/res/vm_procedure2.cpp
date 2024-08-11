//
// Created by Ryan Nicholl on 5/11/24.
//

#include "quxlang/compiler_binding.hpp"
#include "quxlang/res/expr/co_vmir_expression_emitter.hpp"
#include <deque>
#include <quxlang/parsers/parse_expression.hpp>
#include <quxlang/res/vm_procedure2.hpp>

namespace quxlang
{

    class vm_procedure2_generator
    {
        type_symbol func;

        using block_index = std::size_t;

        std::deque< std::string > states;

        vmir2::functanoid_routine2 result;
        compiler* c;

        std::map< std::string, vmir2::storage_index > arg_map;

        std::size_t temp_index = 0;

        std::size_t current_block_index = 0;

        std::map< std::string, block_index > block_map;
        //std::vector< vmir2::executable_block > blocks;

        class binder : compiler_binder
        {
            vm_procedure2_generator* gen;

          public:
            binder(vm_procedure2_generator* gen) : compiler_binder(gen->c), gen(gen)
            {
            }
        };

        template < typename T >
        using co_type = typename compiler_binder::co_type< T >;

        co_type< block_index > generate_entry_block()
        {

            auto index = current_block_index++;

            std::string block_name = "BLOCK" + std::to_string(current_block_index);
            result.blocks.emplace_back();
            block_map[block_name] = index;
            co_return index;
        }

        co_type< block_index > generate_block(block_index parent)
        {
            auto index = current_block_index++;

            std::string block_name;
            block_name = "BLOCK" + std::to_string(index);
            result.blocks.emplace_back();
            block_map[block_name] = index;

            co_return index;
        }

        static co_type< vmir2::storage_index > generate_temporary_f(std::vector< vmir2::vm_slot >& slots, std::size_t& temp_index, type_symbol type)
        {
            slots.push_back(vmir2::vm_slot{.type = std::move(type), .name = "TEMP" + std::to_string(temp_index++), .kind = vmir2::slot_kind::local});
            co_return slots.size() - 1;
        }

        co_type< vmir2::storage_index > generate_temporary(type_symbol type)
        {
            return generate_temporary_f(this->result.slots, this->temp_index, std::move(type));
        }

    
        co_type< void > generate_arg_slots()
        {
            // Precondition: null slot already present
            assert(result.slots.size() == 1);

            // Precondition: Func is a fully instanciated symbol
            instanciation_reference const& inst = as< instanciation_reference >(func);

            std::optional< type_symbol > return_type = co_await QUX_CO_DEP(functanoid_return_type, (inst));

            if (return_type)
            {
                result.slots.push_back(vmir2::vm_slot{.type = create_nslot(*return_type), .name = "RETURN", .kind = vmir2::slot_kind::arg});
                arg_map["RETURN"] = result.slots.size() - 1;
            }

            for (auto const& [name, argtype] : inst.parameters.named_parameters)
            {
                result.slots.push_back(vmir2::vm_slot{.type = argtype, .name = name, .kind = vmir2::slot_kind::arg});
                arg_map[name] = result.slots.size() - 1;
            }

            co_return;
        }

      public:
        vm_procedure2_generator(compiler* carg, type_symbol what) : c(carg), func(what)
        {
        }

        co_type< vmir2::functanoid_routine2 > generate()
        {
            assert(result.slots.size() == 0);
            result.slots.push_back(vmir2::vm_slot{.type = void_type{}, .name = "VOID", .kind = vmir2::slot_kind::literal});

            co_await generate_arg_slots();

            co_await generate_body();

            co_return result;
        }

      private:
        co_type< void > generate_jump(block_index from, block_index to)
        {
            result.blocks.at(from).terminator = vmir2::jump{.target = to};
            co_return;
        }

        co_type< void > generate_branch(block_index from, vmir2::storage_index cond, block_index true_block, block_index false_block)
        {
            result.blocks.at(from).terminator = vmir2::branch{.condition = cond, .target_true = true_block, .target_false = false_block};
            co_return;
        }

        co_type< vmir2::storage_index > generate_bool_expr(block_index current_block, expression const& expr)
        {
            co_return 0;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_if_statement const& st)
        {
            block_index after_block = co_await generate_block(current_block);
            block_index condition_block = co_await generate_block(current_block);
            block_index if_block = co_await generate_block(current_block);

            co_await generate_jump(current_block, condition_block);

            vmir2::storage_index cond = co_await generate_bool_expr(condition_block, st.condition);

            co_await generate_branch(condition_block, cond, if_block, after_block);

            co_await generate_function_block(if_block, st.then_block);
            co_await generate_jump(if_block, after_block);

            if (st.else_block.has_value())
            {
                block_index else_block = co_await generate_block(current_block);
                co_await generate_function_block(else_block, *st.else_block);
                co_await generate_jump(else_block, after_block);
            }
            current_block = after_block;

            co_return;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_while_statement const& st)
        {

            co_return;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_expression_statement const& st)
        {
            co_return;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_block const& st)
        {
            co_return;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_var_statement const& st)
        {
            co_return;
        }

        co_type< void > generate_statement_ovl(block_index& current_block, function_return_statement const& st)
        {

            co_return;
        }

        co_type< void > generate_fblock_statement(block_index& current_block, function_statement const& st)
        {
            co_await rpnx::apply_visitor< co_type< void > >(
                [&](auto st) -> co_type< void >
                {
                    return this->generate_statement_ovl(current_block, st);
                },
                st);
            co_return;
        }

        co_type< void > generate_function_block(block_index &current_block, function_block const& block)
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

        co_type< void > generate_body()
        {
            block_index entry_block = co_await generate_entry_block();

            result.entry_block = entry_block;

            auto inst = as< instanciation_reference >(func);

            auto function_ref_opt = co_await QUX_CO_DEP(functum_select_function, (inst));
            assert(function_ref_opt.has_value());
            auto& function_ref = function_ref_opt.value();

            auto function_decl_opt = co_await QUX_CO_DEP(function_declaration, (function_ref));
            assert(function_decl_opt.has_value());
            ast2_function_declaration& function_decl = function_decl_opt.value();

            co_await generate_function_block(entry_block, function_decl.definition.body);
        }

        co_type< void > generate_return_block()
        {
            block_index return_block = co_await generate_entry_block();
            result.return_block = return_block;
        }
    };

} // namespace quxlang

QUX_CO_RESOLVER_IMPL_FUNC_DEF(vm_procedure2)
{
    vm_procedure2_generator gen(c, input);

    co_return co_await gen.generate();
}
