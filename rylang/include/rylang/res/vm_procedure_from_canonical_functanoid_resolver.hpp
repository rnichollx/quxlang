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
    class vm_procedure_from_canonical_functanoid_resolver : public rpnx::resolver_base< compiler, vm_procedure >
    {
      public:
        using key_type = qualified_symbol_reference;

        vm_procedure_from_canonical_functanoid_resolver(qualified_symbol_reference func_addr)
            : m_func_name(func_addr)
        {
        }

        void process(compiler* c);

      private:
        qualified_symbol_reference m_func_name;

        bool build_generic(compiler* c, vm_generation_frame_info& frame, vm_block& proc, function_statement statement);

        bool build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_var_statement statement);
        bool build(compiler* c, vm_generation_frame_info& frame, vm_block& block, function_if_statement statement);
        bool build(compiler* c, vm_generation_frame_info& frame, vm_block& block, function_while_statement statement);
        bool build(compiler* c, vm_generation_frame_info& frame, vm_block& block, function_return_statement statement);
        bool build(compiler* c, vm_generation_frame_info& frame, vm_block& block, function_expression_statement statement);

        /// Generate a call to a builtin function
        // @param c The compiler
        // @param frame The frame info
        // @param block The vm_block to generate the call in
        // @param callee The function to call
        // @param values The arguments to the function
        // @return A tuple containing the following:
        //  - A boolean that returns true if resolution may proceed
        //  - A boolean that returns true if the call should be a builtin call,
        //    or false if it should be a normal call
        //  - The value of the builtin call, if previous boolean was true
        //    otherwise void_value
        std::tuple< bool, bool, vm_value > try_gen_builtin_call(compiler* c, vm_generation_frame_info& frame, vm_block& block, qualified_symbol_reference callee, std::vector< vm_value > values);



        std::pair< bool, vm_value > gen_call_expr(compiler* c, vm_generation_frame_info& frame, vm_block& block, vm_value callee, std::vector< vm_value > values);

        std::pair<bool, vm_value> gen_call(compiler * c, vm_generation_frame_info & frame, vm_block & block, qualified_symbol_reference callee, std::vector<vm_value> values);

        std::pair<bool, vm_value> gen_call_functanoid(compiler * c, vm_generation_frame_info & frame, vm_block & block, qualified_symbol_reference callee, std::vector<vm_value> values);

        std::pair<bool, vm_value> gen_default_constructor(compiler * c, vm_generation_frame_info & frame, vm_block & block, qualified_symbol_reference callee, std::vector<vm_value> values);

        std::pair<bool, vm_value> gen_invoke(compiler * c, vm_generation_frame_info & frame, vm_block & block, functanoid_reference const& callee, std::vector<vm_value> values);

        std::pair< bool, vm_value > gen_value_generic(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression expr);

        std::pair< bool, std::vector<vm_value>> gen_preinvoke_conversions(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, std::vector<vm_value> values, std::vector<qualified_symbol_reference> const & to_types);

        std::pair< bool, vm_value > gen_implicit_conversion(compiler* c, vm_generation_frame_info& frame, vm_block& block, vm_value from, qualified_symbol_reference to);

        std::pair< bool, vm_value > gen_ref_to_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, vm_value val);

        std::pair< bool, vm_value > gen_value_to_ref(compiler* c, vm_generation_frame_info& frame, vm_block& block, vm_value from, qualified_symbol_reference to_type);

        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_lvalue_reference expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_symbol_reference expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_binary expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_copy_assign expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_call expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, numeric_literal expr);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
