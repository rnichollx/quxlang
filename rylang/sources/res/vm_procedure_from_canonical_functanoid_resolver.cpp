//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/vm_generation_frameinfo.hpp"
#include "rylang/manipulators/mangler.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "rylang/variant_utils.hpp"

// TODO: Debugging, remove this
#include <iostream>

void rylang::vm_procedure_from_canonical_functanoid_resolver::process(compiler* c)
{
    auto function_ast_dp = get_dependency(
        [&]
        {
            return c->lk_function_ast(m_func_name);
        });
    if (!ready())
    {
        return;
    }
    function_ast function_ast_v = function_ast_dp->get();
    vm_procedure vm_proc;
    vm_generation_frame_info frame;
    frame.context = m_func_name;
    vm_proc.interface.return_type = function_ast_v.return_type;
    // First generate the arguments
    for (auto& arg : function_ast_v.args)
    {
        // TODO: consider pass by pointer of large values instead of by value
        vm_frame_variable var;
        // NOTE: Make sure that we can handle contextual types in arg list.
        //  They should be decontextualized somewhere else probably.. so maybe this will
        //  be impossible.
        assert(!qualified_is_contextual(arg.type));
        var.name = arg.name;
        var.type = arg.type;
        frame.variables.push_back(var);
        vm_proc.interface.argument_types.push_back(arg.type);
    }
    // Then generate the body
    for (function_statement const& stmt : function_ast_v.body.statements)
    {
        if (!build_generic(c, frame, vm_proc.body, stmt))
        {
            return;
        }
    }

    set_value(vm_proc);
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build_generic(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_statement statement)
{

    if (typeis< function_var_statement >(statement))
    {
        function_var_statement var_stmt = boost::get< function_var_statement >(statement);
        return build(c, frame, block, var_stmt);
    }
    else if (typeis< function_expression_statement >(statement))
    {
        function_expression_statement expr_stmt = boost::get< function_expression_statement >(statement);
        return build(c, frame, block, expr_stmt);
    }
    else if (typeis< function_if_statement >(statement))
    {
        function_if_statement if_stmt = boost::get< function_if_statement >(statement);
        return build(c, frame, block, if_stmt);
    }
    else if (typeis< function_while_statement >(statement))
    {
        function_while_statement while_stmt = boost::get< function_while_statement >(statement);
        // TODO: Implement
        // return build(c, frame, proc.body, while_stmt);
        assert(false);
    }
    else if (typeis< function_return_statement >(statement))
    {
        function_return_statement return_stmt = boost::get< function_return_statement >(statement);
        return build(c, frame, block, return_stmt);
    }
    else if (typeis< function_block >(statement))
    {
        function_block block_stmt = boost::get< function_block >(statement);
        // TODO: implement
        //return build(c, frame, block, block_stmt);
        assert(false);
    }
    else
    {
        throw std::runtime_error("Unknown function statement type");
    }
    throw std::runtime_error("unimplemented");
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_var_statement statement)
{
    vm_allocate_storage storage;
    auto type_canonical_dp = get_dependency(
        [&]
        {
            return c->lk_canonical_type_from_contextual_type(statement.type, frame.context);
        });
    if (!ready())
    {
        return false;
    }
    qualified_symbol_reference canonical_type = type_canonical_dp->get();
    compiler::out< type_placement_info > type_placement_info_dp = get_dependency(
        [&]
        {
            return c->lk_type_placement_info_from_canonical_type(canonical_type);
        });
    if (!ready())
    {
        return false;
    }
    type_placement_info type_placement_info_v = type_placement_info_dp->get();
    storage.size = type_placement_info_v.size;
    storage.align = type_placement_info_v.alignment;
    vm_frame_variable var;
    var.name = statement.name;
    var.type = canonical_type;
    // TODO: Set destructor here if needed.
    frame.variables.push_back(var);
    block.code.push_back(storage);
    return true;
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                    rylang::function_expression_statement statement)
{
    // TODO: Save frame position
    vm_block expression_block;

    // Each expression needs to have its own block to store temporary lvalues,
    // e.g. a := b + c, creates a temporary lvalue for the result of "b + c" before executing
    // the assignment itself.
    if (auto pair = gen_value_generic(c, frame, expression_block, statement.expr); pair.first)
    {
        vm_execute_expression exc;
        exc.expr = pair.second;
        expression_block.code.push_back(exc);
        block.code.push_back(std::move(expression_block));
        // TODO: Maybe destructor stuff here? think about
        return true;
    }
    else
    {
        return false;
    }
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_generic(compiler* c, vm_generation_frame_info& frame, vm_block& block, expression expr)
{
    if (typeis< expression_lvalue_reference >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_lvalue_reference >(std::move(expr)));
    }
    else if (typeis< expression_copy_assign >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_copy_assign >(std::move(expr)));
    }
    else if (typeis< expression_binary >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_binary >(std::move(expr)));
    }
    else if (typeis< expression_call >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_call >(std::move(expr)));
    }
    else
    {
        // TODO: Add other types
        throw std::logic_error("Unimplemented");
    }
    // gen_expression(c, frame, block, expr);
    return {false, {}};
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                       rylang::expression_lvalue_reference expr)
{
    std::string name = expr.identifier;

    for (int i = 0; i < frame.variables.size(); i++)
    {
        // TODO: Check for duplicates/shadowing
        if (frame.variables[i].name == name)
        {
            vm_expr_load_address load;
            load.index = i;
            auto vartype = frame.variables[i].type;
            // TODO: support type lookup here

            std::string vartype_str = boost::apply_visitor(qualified_symbol_stringifier(), vartype);
            assert(is_canonical(vartype));

            if (is_ref(vartype))
            {
                load.type = vartype;
            }
            else
            {
                // TODO: add support for constant values here
                load.type = make_mref(vartype);
            }
            std::string loadtype_str = boost::apply_visitor(qualified_symbol_stringifier(), load.type);

            return {true, load};
        }
    }

    throw std::logic_error("Couldn't find lvalue in frame");
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                       rylang::expression_copy_assign expr)
{
    // TODO: Support overloading

    auto lhs_pair = gen_value_generic(c, frame, block, expr.lhs);
    auto rhs_pair = gen_value_generic(c, frame, block, expr.rhs);

    if (!lhs_pair.first || !rhs_pair.first)
    {
        return {false, {}};
    }

    // TODO: support operator overloading

    // TODO: Support implicit casts

    vm_value lhs = std::move(lhs_pair.second);
    vm_value rhs = std::move(rhs_pair.second);

    qualified_symbol_reference lhs_type = boost::apply_visitor(vm_value_type_vistor(), lhs);
    std::string lhs_type_string = boost::apply_visitor(qualified_symbol_stringifier(), lhs_type);

    qualified_symbol_reference rhs_type = boost::apply_visitor(vm_value_type_vistor(), rhs);
    std::string rhs_type_string = boost::apply_visitor(qualified_symbol_stringifier(), rhs_type);

    if (!is_ref(lhs_type))
    {
        throw std::runtime_error("Cannot assign to non-reference");
    }

    if (is_ref(rhs_type))
    {
        vm_expr_dereference deref;
        deref.type = remove_ref(rhs_type);
        deref.expr = std::move(rhs);
        rhs_type = deref.type;
        rhs = std::move(deref);
    }

    auto lhs_pointed_type = remove_ref(lhs_type);

    assert(lhs_pointed_type == rhs_type);

    vm_expr_store op;
    op.type = lhs_type;
    op.where = std::move(lhs);
    op.what = std::move(rhs);

    return {true, op};
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_return_statement statement)
{

    if (statement.expr.has_value())
    {
        vm_block expression_block;
        if (auto pair = gen_value_generic(c, frame, expression_block, statement.expr.value()); pair.first)
        {
            vm_return ret;
            ret.expr = pair.second;

            auto vtype = vm_value_type(ret.expr.value());

            // TODO: Support returning references
            if (is_ref(vtype))
            {
                vtype = remove_ref(vtype);
                ret.expr = vm_expr_dereference{ret.expr.value(), vtype};
            }

            expression_block.code.push_back(ret);

            block.code.push_back(std::move(expression_block));
            // TODO: Maybe destructor stuff here? think about
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        block.code.push_back(vm_return{});
        return true;
    }
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_if_statement statement)
{
    vm_block expr_block;
    vm_if if_stmt;

    if (auto pair = gen_value_generic(c, frame, expr_block, statement.condition); pair.first)
    {
        if_stmt.condition = pair.second;
        if (expr_block.code.size() > 0)
        {
            if_stmt.condition_block = std::move(expr_block);
        }
        block.code.push_back(std::move(expr_block));
        for (auto& i : statement.then_block.statements)
        {
            if (!build_generic(c, frame, if_stmt.then_block, i))
            {
                return false;
            }
        }
        if (statement.else_block.has_value())
        {
            if_stmt.else_block = vm_block{};
            for (auto& i : statement.else_block.value().statements)
            {
                if (!build_generic(c, frame, if_stmt.else_block.value(), i))
                {
                    return false;
                }
            }
        }
        block.code.push_back(if_stmt);
        return true;
    }
    else
    {
        return false;
    }
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                       rylang::expression_binary expr)
{

    static std::set< std::string > const bool_operators = {"==", "!=", "<", ">", "<=", ">="};

    static std::set< std::string > const assignment_operators = {":=", ":<"};
    // TODO: Support overloading

    auto lhs_pair = gen_value_generic(c, frame, block, expr.lhs);
    auto rhs_pair = gen_value_generic(c, frame, block, expr.rhs);

    if (!lhs_pair.first || !rhs_pair.first)
    {
        return {false, {}};
    }

    // TODO: support operator overloading

    // TODO: Support implicit casts

    vm_value lhs = std::move(lhs_pair.second);
    vm_value rhs = std::move(rhs_pair.second);

    qualified_symbol_reference lhs_type = boost::apply_visitor(vm_value_type_vistor(), lhs);
    qualified_symbol_reference rhs_type = boost::apply_visitor(vm_value_type_vistor(), rhs);

    auto overload_dp = get_dependency(
        [&]
        {
            return c->lk_operator_is_overloaded_with(expr.operator_str, vm_value_type(lhs), vm_value_type(rhs));
        });
    if (!ready())
    {
        return {false, {}};
    }
    std::optional< qualified_symbol_reference > overloaded_operator = overload_dp->get();

    if (overloaded_operator.has_value())
    {
        std::string overloaded_operator_string = mangle(overloaded_operator.value());

        return gen_call(c, frame, block, overloaded_operator_string, {lhs, rhs});
    }

    if (is_ref(lhs_type) && assignment_operators.count(expr.operator_str) == 0)
    // Do not dereference for assignment operators
    {
        vm_expr_dereference deref;
        deref.type = remove_ref(lhs_type);
        deref.expr = std::move(lhs);
        lhs_type = deref.type;
        lhs = std::move(deref);
    }

    if (is_ref(rhs_type))
    {
        vm_expr_dereference deref2;
        deref2.type = remove_ref(rhs_type);
        deref2.expr = std::move(rhs);
        rhs_type = deref2.type;
        rhs = std::move(deref2);
    }

    vm_expr_primitive_binary_op op;
    op.oper = expr.operator_str;
    if (bool_operators.count(expr.operator_str) > 0)
    {
        op.type = qualified_symbol_reference{primitive_type_bool_reference{}};
    }
    else
    {
        op.type = lhs_type;
    }
    op.lhs = std::move(lhs);
    op.rhs = std::move(rhs);

    std::string lhs_type_string = boost::apply_visitor(qualified_symbol_stringifier(), lhs_type);
    std::string rhs_type_string = boost::apply_visitor(qualified_symbol_stringifier(), rhs_type);

    std::string lhs_type_string2 = to_string(vm_value_type(op.lhs));
    std::string rhs_type_string2 = to_string(vm_value_type(op.rhs));

    std::cout << "lhs_type_string: " << lhs_type_string << std::endl;
    std::cout << "rhs_type_string: " << rhs_type_string << std::endl;

    std::cout << "lhs_type_string2: " << lhs_type_string2 << std::endl;
    std::cout << "rhs_type_string2: " << rhs_type_string2 << std::endl;

    std::cout << "lhs: " << to_string(op.lhs) << std::endl;
    std::cout << "rhs: " << to_string(op.rhs) << std::endl;

    std::cout << "Generated the following binop: " << to_string(op) << std::endl;
    //    assert(remove_ref(lhs_type) == rhs_type);

    return {true, op};
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                       rylang::expression_call expr)
{
    vm_expr_call call;
    for (auto & arg : expr.args)
    {
        // TODO: Translate arg types from references

        auto pv = gen_value_generic(c, frame, block, arg);
        bool ok = pv.first;
        if (!ok)
        {
            return {false, {}};
        }

        call.arguments.push_back(pv.second);

    }


    return {true, call};
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                      std::string funcname_mangled, std::vector< vm_value > args)
{
    // TODO
    assert(false);
    return std::pair< bool, vm_value >();
}
