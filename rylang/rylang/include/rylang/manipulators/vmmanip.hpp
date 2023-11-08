//
// Created by Ryan Nicholl on 11/7/23.
//

#ifndef RPNX_RYANSCRIPT1031_VMMANIP_HEADER
#define RPNX_RYANSCRIPT1031_VMMANIP_HEADER

#include "rylang/data/qualified_reference.hpp"
#include "rylang/data/vm_expr_primitive_op.hpp"
#include "rylang/data/vm_expression.hpp"
#include <boost/variant.hpp>

namespace rylang
{
    struct vm_value_type_vistor : boost::static_visitor< qualified_symbol_reference >
    {
        qualified_symbol_reference operator()(std::monostate) const
        {
            return {};
        }
        qualified_symbol_reference operator()(vm_expr_primitive_binary_op const& op) const
        {
            return op.type;
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

        std::string operator()(vm_return what) const
        {
            std::string result = "return";
            if (what.expr)
            {

                result += "(" + to_string(*what.expr) + ")";
            }
            return result;
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
          return "binary_op<" + to_string(exp.type) + ">(" + to_string(exp.lhs) + ", " + to_string(exp.rhs) + ")";
        }

        std::string operator()(vm_expr_primitive_unary_op const& exp) const
        {
          return "unary_op<" + to_string(exp.type) + ">(" + to_string(exp.expr) + ")";
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
