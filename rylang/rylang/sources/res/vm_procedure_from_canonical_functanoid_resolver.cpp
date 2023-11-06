//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/vm_generation_frameinfo.hpp"
#include "rylang/manipulators/qualified_reference.hpp"
#include "rylang/variant_utils.hpp"

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

    vm_proc.interface.return_type2 = function_ast_v.return_type;

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
        vm_proc.interface.argument_types2.push_back(arg.type);
    }

    // Then generate the body
    for (function_statement const& stmt : function_ast_v.body.statements)
    {
        if (!build(c, frame, vm_proc, stmt))
        {
            return;
        }
    }

    set_value(vm_proc);
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_procedure& proc, rylang::function_statement statement)
{
    if (typeis< function_var_statement >(statement))
    {
        function_var_statement var_stmt = boost::get< function_var_statement >(statement);
        return build(c, frame, proc, var_stmt);
    }
    else if (typeis< function_expression_statement >(statement))
    {
        function_expression_statement expr_stmt = boost::get< function_expression_statement >(statement);
        return build(c, frame, proc, expr_stmt);
    }
    else if (typeis< function_if_statement >(statement))
    {
        function_if_statement if_stmt = boost::get< function_if_statement >(statement);
        return build(c, frame, proc, if_stmt);
    }
    else if (typeis< function_while_statement >(statement))
    {
        function_while_statement while_stmt = boost::get< function_while_statement >(statement);
        return build(c, frame, proc, while_stmt);
    }
    else if (typeis< function_return_statement >(statement))
    {
        function_return_statement return_stmt = boost::get< function_return_statement >(statement);
        return build(c, frame, proc, return_stmt);
    }
    else if (typeis< function_block >(statement))
    {
        function_block block_stmt = boost::get< function_block >(statement);
        return build(c, frame, proc, block_stmt);
    }
    else
    {
        throw std::runtime_error("Unknown function statement type");
    }
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
    if (auto pair = gen_value(c, frame, expression_block, statement.expr); pair.first)
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
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                                       rylang::expression expr)
{
    if (typeis< expression_lvalue_reference >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_lvalue_reference >(std::move(expr)));
    }
    else if (typeis< expression_copy_assign >(expr))
    {
       return gen_value(c, frame, block, boost::get< expression_copy_assign >(std::move(expr)));
    }
    // gen_expression(c, frame, block, expr);
    return {false, {}};
}
std::pair< bool, vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block,
                                                                                               rylang::expression_lvalue_reference expr)
{
    bool expression_result_is_value = true;
    // TODO: Handle references/rvalues/lvalues etc.
    //  For now, just assume the result is a value and nothing bindable is returned...

    std::string name = expr.identifier;

    for (int i = 0; i < frame.variables.size(); i++)
    {
        if (frame.variables[i].name == name)
        {
        }
    }

    }
}
