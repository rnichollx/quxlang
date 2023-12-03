//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/vm_procedure.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/function_statement.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/vm_generation_frameinfo.hpp"

namespace rylang
{
    class vm_procedure_from_canonical_functanoid_resolver : public rpnx::co_resolver_base< compiler, vm_procedure, qualified_symbol_reference >
    {
      public:
        vm_procedure_from_canonical_functanoid_resolver(qualified_symbol_reference func_addr)
            : rpnx::co_resolver_base< compiler, vm_procedure, qualified_symbol_reference >(func_addr)
        {
        }

        virtual rpnx::resolver_coroutine< compiler, vm_procedure > co_process(compiler* c, qualified_symbol_reference func_addr) override final;

      private:
        struct context_frame
        {
            std::size_t exception_ct = 0;
            bool closed = false;

          public:
            context_frame(vm_procedure_from_canonical_functanoid_resolver* resolver, qualified_symbol_reference func, compiler* c, vm_generation_frame_info& frame, vm_procedure& block);
            explicit context_frame(context_frame& other);

            struct condition_t
            {
            };
            struct then_t
            {
            };
            struct else_t
            {
            };
            struct loop_t
            {
            };

            static constexpr condition_t condition_tag = condition_t{};
            static constexpr then_t then_tag = then_t{};
            static constexpr else_t else_tag = else_t{};
            static constexpr loop_t loop_tag = loop_t{};

            void comment(std::string const& str);

            explicit context_frame(context_frame& other, vm_if& insertion_point, condition_t);
            explicit context_frame(context_frame& other, vm_if& insertion_point, then_t);
            explicit context_frame(context_frame& other, vm_if& insertion_point, else_t);
            explicit context_frame(context_frame& other, vm_while& insertion_point, condition_t);
            explicit context_frame(context_frame& other, vm_while& insertion_point, loop_t);
            context_frame(context_frame&& other) = delete;

            ~context_frame() noexcept(false);

            rpnx::general_coroutine< compiler, std::monostate > close();
            rpnx::general_coroutine< compiler, std::monostate > discard();

            [[nodiscard]] inline rpnx::general_coroutine< compiler, std::size_t > create_variable_storage(std::string name, qualified_symbol_reference type);
            [[nodiscard]] inline rpnx::general_coroutine< compiler, std::size_t > create_value_storage(std::optional< std::string > name, qualified_symbol_reference type);
            [[nodiscard]] inline rpnx::general_coroutine< compiler, std::size_t > create_temporary_storage(qualified_symbol_reference type);
            [[nodiscard]] vm_value load_temporary(std::size_t index);
            [[nodiscard]] vm_value load_temporary_as_new(std::size_t index);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > set_return_value(vm_value);
            [[nodiscard]] inline vm_value load_variable(std::size_t index)
            {
                return load_value(index, true, false);
            }
            [[nodiscard]] inline vm_value load_variable_as_new(std::size_t index)
            {
                return load_value(index, false, false);
            }
            void set_value_alive(std::size_t index);
            void set_value_dead(std::size_t index);
            [[nodiscard]] std::size_t construct_new_temporary(qualified_symbol_reference type, std::vector< vm_value > args);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::size_t > adopt_value_as_temporary(vm_value val);
            [[nodiscard]] std::optional< std::size_t > try_get_variable_index(std::string name);
            [[nodiscard]] qualified_symbol_reference get_variable_type(std::size_t index);
            [[nodiscard]] std::optional< qualified_symbol_reference > try_get_variable_type(std::string name);

            [[nodiscard]] std::optional< vm_value > try_load_variable(std::string name);
            [[nodiscard]] vm_value load_value(std::size_t index, bool alive, bool temp);
            [[nodiscard]] vm_value load_value_as_desctructable(std::size_t index);

            [[nodiscard]] vm_value load_variable(std::string name);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > construct_new_variable(std::string name, qualified_symbol_reference type, std::vector< vm_value > args);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > destroy_value(std::size_t index);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > frame_return(vm_value val);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > frame_return();
            [[nodiscard]] rpnx::general_coroutine< compiler, void > run_value_destructor(std::size_t index);
            [[nodiscard]] rpnx::general_coroutine< compiler, std::monostate > run_value_constructor(std::size_t index, std::vector< vm_value > args);

            vm_procedure& procedure()
            {
                return m_proc;
            }

            vm_procedure const& procedure() const
            {
                return m_proc;
            }

          public:
            qualified_symbol_reference current_context() const;

            void push(vm_executable_unit s)
            {
                m_new_block.code.push_back(std::move(s));
            }

            class compiler* compiler() const
            {
                return m_c;
            }

          private:
          public:
            qualified_symbol_reference funcname()
            {
                return m_frame.context;
            }

            class compiler* m_c;
            vm_generation_frame_info& m_frame;
            qualified_symbol_reference m_ctx;
            // vm_block& m_block;
            vm_block m_new_block;
            std::function< void(vm_block) > m_insertion_point;
            vm_procedure_from_canonical_functanoid_resolver* m_resolver;

            vm_procedure& m_proc;
        };

        [[nodiscard]] rpnx::general_coroutine< compiler, void > build_generic(context_frame& ctx, function_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, rylang::function_var_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, function_if_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, function_while_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, function_return_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, function_expression_statement statement);
        [[nodiscard]] rpnx::general_coroutine< compiler, void > build(context_frame& ctx, function_block statement);

        vm_value gen_conversion_to_integer(context_frame& ctx, vm_expr_literal val, primitive_type_integer_reference to_type);
        rpnx::general_coroutine< compiler, vm_value > gen_call_expr(context_frame& ctx, vm_value callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, vm_value > gen_call(context_frame& ctx, qualified_symbol_reference callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, std::optional< vm_value > > try_gen_call_functanoid_builtin(context_frame& ctx, qualified_symbol_reference callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, vm_value > gen_call_functanoid(context_frame& ctx, qualified_symbol_reference callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, vm_value > gen_default_constructor(context_frame& ctx, qualified_symbol_reference callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, vm_value > gen_default_destructor(context_frame& ctx, qualified_symbol_reference callee, std::vector< vm_value > values);

        rpnx::general_coroutine< compiler, vm_value > gen_invoke(context_frame& ctx, functanoid_reference const& callee, std::vector< vm_value > values);
        rpnx::general_coroutine< compiler, vm_value > gen_value_generic(context_frame& ctx, expression expr);
        rpnx::general_coroutine< compiler, std::vector< vm_value > > gen_preinvoke_conversions(context_frame& ctx, std::vector< vm_value > values, std::vector< qualified_symbol_reference > const& to_types);
        rpnx::general_coroutine< compiler, vm_value > gen_implicit_conversion(context_frame& ctx, vm_value from, qualified_symbol_reference to);
        rpnx::general_coroutine< compiler, vm_value > gen_ref_to_value(context_frame& ctx, vm_value val);
        rpnx::general_coroutine< compiler, vm_value > gen_value_to_ref(context_frame& ctx, vm_value from, qualified_symbol_reference to_type);
        rpnx::general_coroutine< compiler, vm_value > gen_value(context_frame& ctx, expression_symbol_reference expr);
        rpnx::general_coroutine< compiler, vm_value > gen_value(context_frame& ctx, expression_binary expr);
        rpnx::general_coroutine< compiler, vm_value > gen_value(context_frame& ctx, expression_copy_assign expr);
        rpnx::general_coroutine< compiler, vm_value > gen_value(context_frame& ctx, expression_call expr);
        rpnx::general_coroutine< compiler, vm_value > gen_value(context_frame& ctx, numeric_literal expr);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
