//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
#define RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER

#include "rylang/compiler_fwd.hpp"
#include "rylang/data/vm_procedure.hpp"

#include "rpnx/resolver_utilities.hpp"
#include "rylang/data/function_statement.hpp"
#include "rylang/data/qualified_reference.hpp"
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

        std::pair< bool, vm_value > gen_call(compiler* c, vm_generation_frame_info& frame, vm_block& block, std::string funcname_mangled, std::vector< vm_value > args);

        std::pair< bool, vm_value > gen_value_generic(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression expr);

        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_lvalue_reference expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_binary expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_copy_assign expr);
        std::pair< bool, vm_value > gen_value(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression_call expr);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_PROCEDURE_FROM_CANONICAL_FUNCTANOID_RESOLVER_HEADER
