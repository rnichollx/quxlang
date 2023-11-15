//
// Created by Ryan Nicholl on 11/11/23.
//
#include "rylang/res/function_frame_information_resolver.hpp"
#include "rylang/ast/function_ast.hpp"
#include "rylang/compiler.hpp"
#include "rylang/variant_utils.hpp"

void rylang::function_frame_information_resolver::process(compiler* c)
{
    auto function_ast_dp = get_dependency(
        [&]
        {
            return c->lk_function_ast(m_input);
        });
    if (!ready())
    {
        return;
    }
    function_ast function_ast_v = function_ast_dp->get();

    function_frame_information frame;

    recurse(c, frame, function_ast_v.body);
}

bool rylang::function_frame_information_resolver::recurse(compiler* c, function_frame_information& frame, function_if_statement statement)
{
    assert(false);
    // TODO: Implement
}

bool rylang::function_frame_information_resolver::recurse(compiler* c, function_frame_information& frame, function_while_statement statement)
{
    assert(false);
    // TODO: implement
}

bool rylang::function_frame_information_resolver::recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_block const& block)
{
    for (function_statement const& st : block.statements)
    {
        if (typeis< function_var_statement >(st))
        {
            function_var_statement const& var_st = boost::get< function_var_statement >(st);
            if (!recurse(c, frame, var_st))
            {
                return false;
            }
        }
        else if (typeis< function_while_statement >(st))
        {
            function_while_statement const& while_st = boost::get< function_while_statement >(st);
            if (!recurse(c, frame, while_st))
            {
                return false;
            }
        }
        else if (typeis< function_if_statement >(st))
        {
            function_if_statement const& if_st = boost::get< function_if_statement >(st);
            if (!recurse(c, frame, if_st))
            {
                return false;
            }
        }
        // TODO: implement
        assert(false);
    }

    return true;
}

bool rylang::function_frame_information_resolver::recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_var_statement const& statement)
{
    function_allocation_information alloc_info;

    auto type_dp = get_dependency(
        [&]
        {
            return c->lk_canonical_type_from_contextual_type(statement.type, m_input);
        });

    if (!ready())
    {
        return false;
    }

    qualified_symbol_reference canonical_var_type = type_dp->get();

    auto placement_dp = get_dependency(
        [&]
        {
            return c->lk_type_placement_info_from_canonical_type(canonical_var_type);
        });

    if (!ready())
    {
        return false;
    }

    type_placement_info placement = placement_dp->get();

    alloc_info.size = placement.size;
    alloc_info.alignment = placement.alignment;
    alloc_info.type = canonical_var_type;

    frame.allocations.push_back(alloc_info);

    auto frame_index = frame.allocations.size() - 1;

    function_variable_information var_info;
    var_info.identifier = statement.name;
    var_info.type = canonical_var_type;
    var_info.allocation = frame_index;

    frame.variables.push_back(var_info);

    return true;
}

bool rylang::function_frame_information_resolver::recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_return_statement const& statement)
{
    // TODO: Implement
    return false;
}

bool rylang::function_frame_information_resolver::recurse(rylang::compiler* c, rylang::function_frame_information& frame, rylang::function_expression_statement const& statement)
{
    // TODO: Implement
    return false;
}
