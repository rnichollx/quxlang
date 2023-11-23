//
// Created by Ryan Nicholl on 11/7/23.
//

#ifndef RPNX_RYANSCRIPT1031_VMMANIP_HEADER
#define RPNX_RYANSCRIPT1031_VMMANIP_HEADER

#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/data/vm_expr_primitive_op.hpp"
#include "rylang/data/vm_expression.hpp"
#include <boost/variant.hpp>

namespace rylang
{
    struct vm_value_type_vistor : boost::static_visitor< qualified_symbol_reference >
    {
        qualified_symbol_reference operator()(void_value const&) const
        {
            return void_type{};
        }
        qualified_symbol_reference operator()(vm_expr_primitive_binary_op const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_access_field const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_bound_value const& op) const
        {
            bound_function_type_reference result;
            result.function_type = op.function_ref;
            result.object_type = boost::apply_visitor(vm_value_type_vistor{}, op.value);
            return result;
        }

        qualified_symbol_reference operator()(vm_expr_primitive_unary_op const& op) const
        {
            return op.type;
        }
        qualified_symbol_reference operator()(vm_expr_load_address const& op) const
        {
            return op.type;
        }
        qualified_symbol_reference operator()(vm_expr_dereference const& op) const
        {
            return op.type;
        }
        qualified_symbol_reference operator()(vm_expr_store const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_call const& op) const
        {
            return op.interface.return_type.value_or(void_type{});
        }

        qualified_symbol_reference operator()(vm_expr_load_literal const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_literal const& op) const
        {
            return numeric_literal_reference{};
        }

        qualified_symbol_reference operator()(vm_expr_reinterpret const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_poison const& op) const
        {
            return op.type;
        }

        qualified_symbol_reference operator()(vm_expr_undef const& op) const
        {
            return op.type;
        }

      public:
        vm_value_type_vistor() = default;
    };

    inline qualified_symbol_reference vm_value_type(vm_value const& val)
    {
        return boost::apply_visitor(vm_value_type_vistor{}, val);
    }

    std::string to_string(vm_value const&);
    std::string to_string(vm_executable_unit const&);

    struct vm_expression_stringifier : boost::static_visitor< std::string >
    {
        std::string operator()(vm_expr_literal lit) const
        {
            return "LITERAL(" + lit.literal + ")";
        }

        std::string operator()(vm_expr_store what) const
        {
            std::string store_type;
            std::string stored_type;

            store_type = to_string(vm_value_type(what.what));
            stored_type = to_string(vm_value_type(what.where));

            std::string result = "store<" + store_type + ", " + stored_type + ">(what=";
            result += to_string(what.what) + ", where=" + to_string(what.where) + ")";

            return result;
        }

        std::string operator()(vm_expr_bound_value what) const
        {
            std::string result = "bound_value<" + to_string(what.function_ref) + ">(" + to_string(what.value) + ")";
            return result;
        }

        std::string operator()(vm_expr_load_literal lit) const
        {
            return "LOAD_LITERAL<" + to_string(lit.type) + ">(" + lit.literal + ")";
        }

        std::string operator()(vm_return what) const
        {
            std::string result = "RETURN";
            return result;
        }

        std::string operator()(void_value) const
        {
            return "VOID()";
        }

        std::string operator()(vm_execute_expression what) const
        {
            return "execute(" + to_string(what.expr) + ")";
        }

        std::string operator()(vm_block const& what) const
        {
            std::string result = "block{";
            for (auto const& i : what.code)
            {
                result += to_string(i) + "; ";
            }
            result += ")";
            return result;
        }

        std::string operator()(vm_allocate_storage what) const
        {
            return "allocate_storage<" + to_string(what.type) + ">(size=" + std::to_string(what.size) + ", align= " + std::to_string(what.align) + ")";
        }

        std::string operator()(vm_store what) const
        {
            return "store<" + to_string(what.type) + ">(what=" + to_string(what.what) + ", where=" + to_string(what.where) + ")";
        }

        std::string operator()(vm_expr_dereference what) const
        {
            return "deref_expr<" + to_string(what.type) + ">(expr=" + to_string(what.expr) + ")";
        }

        std::string operator()(vm_expr_load_address what) const
        {
            return "load_addr<" + to_string(what.type) + ">(" + std::to_string(what.index) + ")";
        }

        std::string operator()(std::monostate const&) const
        {
            return "ERROR(monostate)";
        }

        std::string operator()(vm_expr_primitive_binary_op const& exp) const
        {
            return "binary_op " + exp.oper + " <" + to_string(exp.type) + ">(" + to_string(exp.lhs) + ", " + to_string(exp.rhs) + ")";
        }

        std::string operator()(vm_expr_access_field const& exp) const
        {
            return "access_field<" + to_string(exp.type) + ">(" + to_string(exp.base) + ", +" + std::to_string(exp.offset) + ")";
        }

        std::string operator()(vm_expr_primitive_unary_op const& exp) const
        {
            return "unary_op<" + to_string(exp.type) + ">(" + to_string(exp.expr) + ")";
        }

        std::string operator()(vm_if const& ifi) const
        {
            std::string result = "if(" + to_string(ifi.condition) + ") then " + to_string(ifi.then_block);
            if (ifi.else_block)
            {
                result += " else " + to_string(*ifi.else_block);
            }
            return result;
        }

        std::string operator()(vm_while const& wfi) const
        {
            std::string result = "while(" + to_string(wfi.condition) + ") do " + to_string(wfi.loop_block);
            return result;
        }

        std::string operator()(vm_expr_call const& obj) const
        {
            std::string result = "call<" + to_string(obj.functanoid) + ">[" + obj.mangled_procedure_name + "](";
            for (int i = 0; i < obj.arguments.size(); i++)
            {
                if (i != 0)
                    result += ", ";
                result += to_string(obj.arguments[i]);
            }
            result += ")";
            return result;
        }

        std::string operator()(vm_expr_reinterpret const& obj) const
        {
            return "reinterpret<" + to_string(obj.type) + ">(" + to_string(obj.expr) + ")";
        }

        std::string operator()(vm_expr_poison const& obj) const
        {
            return "poison<" + to_string(obj.type) + ">()";
        }

        std::string operator()(vm_expr_undef const& obj) const
        {
            return "undef<" + to_string(obj.type) + ">()";
        }

        std::string operator()(vm_disable_storage const&) const
        {
            assert(false); // TODO: implement this
            return "";
        }

        std::string operator()(vm_enable_storage const&) const
        {
            assert(false); // TODO: implement this
            return "";
        }

        vm_expression_stringifier() = default;
    };

    inline std::string to_string(vm_value const& val)
    {
        return boost::apply_visitor(vm_expression_stringifier{}, val);
    }
    inline std::string to_string(vm_executable_unit const& val)
    {
        return boost::apply_visitor(vm_expression_stringifier{}, val);
    }
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VMMANIP_HEADER
