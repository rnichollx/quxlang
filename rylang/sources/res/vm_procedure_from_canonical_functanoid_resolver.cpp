//
// Created by Ryan Nicholl on 11/5/23.
//
#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/vm_generation_frameinfo.hpp"
#include "rylang/manipulators/mangler.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "rylang/operators.hpp"
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

        return build(c, frame, block, while_stmt);
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
        // return build(c, frame, block, block_stmt);
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

    std::string var_type_str = to_string(statement.type);

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

    std::string canonical_var_str = to_string(canonical_type);

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
    var.get_addr = vm_expr_load_address{frame.variables.size(), make_mref(canonical_type)};
    // TODO: Set destructor here if needed.
    frame.variables.push_back(var);
    block.code.push_back(storage);
    subdotentity_reference constructor_symbol = {canonical_type, "CONSTRUCTOR"};
    gen_call(c, frame, block, constructor_symbol, std::vector< vm_value >{var.get_addr.value()});

    return true;
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_expression_statement statement)
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
    if (typeis< expression_symbol_reference >(expr))
    {
        return gen_value(c, frame, block, boost::get< expression_symbol_reference >(std::move(expr)));
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
    else if (typeis< numeric_literal >(expr))
    {
        return gen_value(c, frame, block, boost::get< numeric_literal >(std::move(expr)));
    }
    else
    {
        // TODO: Add other types
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }
    // gen_expression(c, frame, block, expr);
    return {false, {}};
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::expression_lvalue_reference expr)
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
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::expression_copy_assign expr)
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
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::expression_binary expr)
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
    qualified_symbol_reference rhs_type = boost::apply_visitor(vm_value_type_vistor(), rhs);

    qualified_symbol_reference lhs_underlying_type = remove_ref(lhs_type);
    qualified_symbol_reference rhs_underlying_type = remove_ref(rhs_type);

    qualified_symbol_reference lhs_function = subdotentity_reference{lhs_underlying_type, "OPERATOR" + expr.operator_str};
    qualified_symbol_reference rhs_function = subdotentity_reference{rhs_underlying_type, "OPERATOR" + expr.operator_str + "RHS"};

    call_parameter_information lhs_param_info{{lhs_type, rhs_type}};
    call_parameter_information rhs_param_info{{rhs_type, lhs_type}};
    auto lhs_exists_and_callable_with_dp = get_dependency(
        [&]
        {
            return c->lk_functum_exists_and_is_callable_with(lhs_function, lhs_param_info);
        });

    if (!ready())
    {
        return {false, {}};
    }

    bool lhs_exists_and_callable_with = lhs_exists_and_callable_with_dp->get();

    if (lhs_exists_and_callable_with)
    {
        return gen_call(c, frame, block, lhs_function, std::vector< vm_value >{lhs, rhs});
    }

    auto rhs_exists_and_callable_with_dp = get_dependency(
        [&]
        {
            return c->lk_functum_exists_and_is_callable_with(rhs_function, rhs_param_info);
        });

    if (!ready())
    {
        return {false, {}};
    }

    bool rhs_exists_and_callable_with = rhs_exists_and_callable_with_dp->get();

    if (rhs_exists_and_callable_with)
    {
        return gen_call(c, frame, block, rhs_function, std::vector< vm_value >{rhs, lhs});
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::expression_call expr)
{
    auto callee_pv = gen_value_generic(c, frame, block, expr.callee);
    bool ok = callee_pv.first;

    if (!ok)
    {
        return {false, {}};
    }

    std::vector< vm_value > args;

    for (auto& arg : expr.args)
    {
        // TODO: Translate arg types from references

        auto pv = gen_value_generic(c, frame, block, arg);
        ok = pv.first;
        if (!ok)
        {
            return {false, {}};
        }
        args.push_back(pv.second);
    }

    return gen_call_expr(c, frame, block, callee_pv.second, args);
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::expression_symbol_reference expr)
{
    bool is_possibly_frame_value = typeis< subentity_reference >(expr.symbol) && typeis< context_reference >(boost::get< subentity_reference >(expr.symbol).parent);

    // Frame values are sub-entities of the current context.
    if (is_possibly_frame_value)
    {
        std::string name = boost::get< subentity_reference >(expr.symbol).subentity_name;
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
    }

    assert(frame.context == m_func_name);
    auto canonical_symbol_dp = get_dependency(
        [&]
        {
            return c->lk_canonical_type_from_contextual_type(expr.symbol, m_func_name);
        });

    if (!ready())
    {
        return {false, {}};
    }

    qualified_symbol_reference canonical_symbol = canonical_symbol_dp->get();

    std::string symbol_str = to_string(canonical_symbol);
    // TODO: Check if global variable
    bool is_global_variable = false;
    bool is_function = true; // This might not actually be true

    vm_expr_bound_value result;
    result.function_ref = canonical_symbol;
    result.value = void_value{};

    return {true, result};
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_implicit_conversion(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::vm_value from, rylang::qualified_symbol_reference to)
{
    auto from_type = vm_value_type(from);

    if (from_type == to)
    {
        return {true, from};
    }

    if (remove_ref(from_type) == to)
    {
        return gen_ref_to_value(c, frame, block, from);
    }

    else if (from_type == remove_ref(to))
    {
        return gen_value_to_ref(c, frame, block, from, to);
    }

    if (is_ref(from_type) && is_ref(to) && remove_ref(from_type) == remove_ref(to))
    {
        return {true, vm_expr_reinterpret{from, to}};
    }

    auto underlying_to_type = remove_ref(to);
    if (typeis< primitive_type_integer_reference >(to) && typeis< vm_expr_literal >(from))
    {
        auto result = gen_conversion_to_integer(c, frame, block, boost::get< vm_expr_literal >(from), boost::get< primitive_type_integer_reference >(to));

        if (is_ref(to))
        {
            return gen_value_to_ref(c, frame, block, result, to);
        }
        else
        {
            return {true, result};
        }
    }

    // TODO: Allowed integer conversions, etc

    throw std::runtime_error("Cannot convert between these types");
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_to_ref(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::vm_value from, rylang::qualified_symbol_reference to_type)
{

    assert(is_canonical(to_type));
    auto result_placement_info_dp = get_dependency(
        [&]
        {
            return c->lk_type_placement_info_from_canonical_type(to_type);
        });
    if (!ready())
    {
        return {false, {}};
    }
    type_placement_info result_placement_info = result_placement_info_dp->get();
    vm_allocate_storage storage;
    storage.type = to_type;
    storage.size = result_placement_info.size;
    storage.align = result_placement_info.alignment;
    block.code.push_back(storage);
    auto index = frame.variables.size();
    vm_frame_variable var;
    var.name = "TEMPORARY";
    var.type = to_type;
    frame.variables.push_back(var);
    vm_expr_load_address load;
    load.index = index;
    load.type = to_type;
    vm_expr_store store;
    store.type = to_type;
    store.where = load;
    store.what = from;
    vm_execute_expression expr;
    expr.expr = store;
    block.code.push_back(expr);
    return {true, load};
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_ref_to_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::vm_value val)
{
    auto arg_type = vm_value_type(val);
    qualified_symbol_reference to_type = remove_ref(arg_type);

    assert(is_canonical(arg_type));
    // TODO: Copy constructor, if needed, etc.
    return {true, vm_expr_dereference{val, to_type}};
}

std::tuple< bool, bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::try_gen_builtin_call(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::qualified_symbol_reference callee, std::vector< vm_value > values)
{
    // This should be unwrapped by the caller
    assert(!typeis< bound_function_type_reference >(callee));

    if (typeis< subdotentity_reference >(callee))
    {
        subdotentity_reference const& subdot = boost::get< subdotentity_reference >(callee);
        qualified_symbol_reference parent_type = subdot.parent;

        assert(!values.empty());

        // if (subdot.subdotentity_name.starts_with("OPERATOR") && typeis< primitive_type_integer_reference >(parent_type))
        //{
        //   primitive_type_integer_reference const& int_type = boost::get< primitive_type_integer_reference >(parent_type);

        //}
        if (subdot.subdotentity_name == "CONSTRUCTOR" && typeis< primitive_type_integer_reference >(parent_type))
        {
            primitive_type_integer_reference const& int_type = boost::get< primitive_type_integer_reference >(parent_type);

            // Can't call this... not possible
            if (values.empty())
            {
                throw std::runtime_error("Cannot call member function with no parameters (requires at least 'this' parameter)");
            }

            if (values.size() > 2)
            {
                throw std::runtime_error("Invalid number of arguments to integer constructor");
            }

            vm_value arg = values[0];

            qualified_symbol_reference arg_type = vm_value_type(arg);

            qualified_symbol_reference arg_type_underlying = remove_ref(arg_type);
            if (arg_type_underlying != qualified_symbol_reference(int_type))
            {
                throw std::runtime_error("Member constructor cannot be used with unrelated type (note: T::.CONSTRUCTOR(...)'s first parameter is a reference to the storage for the object to construct. Maybe you meant to use the free constructor instead? i.e. T::CONSTRUCTOR(...)? (no dot after ::)");
            }

            if (!is_ref(arg_type))
            {
                throw std::runtime_error("Cannot call constructor on a value (THIS must be a reference)");
            }

            if (typeis< cvalue_reference >(arg_type))
            {
                throw std::runtime_error("Called constructor cannot mutate a constant reference 'THIS' value (THIS must be MUT& or OUT& reference)");
            }

            if (!typeis< primitive_type_integer_reference >(arg_type_underlying))
            {
                throw std::runtime_error("Invalid argument type to integer constructor");
            }

            auto int_arg_type = boost::get< primitive_type_integer_reference >(arg_type_underlying);
            if (int_arg_type != int_type)
            {
                throw std::runtime_error("Unimplemented integer of different type passed to int constructor");
            }

            if (values.size() == 1)
            {
                // default constructor

                vm_expr_store initalizer;

                initalizer.what = vm_expr_load_literal{"0", int_type};
                initalizer.where = arg;
                initalizer.type = int_type;
                block.code.push_back(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }

            else if (values.size() == 2)
            {
                // copy constructor

                vm_expr_store initalizer;

                vm_value arg_to_copy = values.at(1);
                qualified_symbol_reference arg_copy_type = vm_value_type(arg_to_copy);
                // TODO: conversion to integer?

                if (is_ref(vm_value_type(arg_to_copy)))
                {
                    // TODO: Check parameter is a valid input (i.e. not an output ref)

                    arg_to_copy = vm_expr_dereference{arg_to_copy, remove_ref(vm_value_type(arg_to_copy))};
                    arg_copy_type = remove_ref(arg_copy_type);
                }

                initalizer.what = arg_to_copy;
                initalizer.where = arg;
                initalizer.type = int_type;
                block.code.push_back(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }
        }
        else if (subdot.subdotentity_name == "CONSTRUCTOR")
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen_dp = get_dependency(
                [&]
                {
                    return c->lk_class_should_autogen_default_constructor(parent_type);
                });
            if (!ready())
            {
                return {false, false, {}};
            }
            bool should_autogen = should_autogen_dp->get();
            if (!should_autogen)
            {
                return {true, false, {}};
            }

            auto pair = gen_default_constructor(c, frame, block, parent_type, values);
            if (pair.first)
            {
                return {true, true, pair.second};
            }
            else
            {
                return {false, {}, {}};
            }
        }
    }

    if (typeis< subentity_reference >(callee))
    {
        subentity_reference const& sr = boost::get< subentity_reference >(callee);

        qualified_symbol_reference parent = sr.parent;
        if (sr.subentity_name == "CONSTRUCTOR" && typeis< primitive_type_integer_reference >(parent))
        {
            primitive_type_integer_reference const& int_type = boost::get< primitive_type_integer_reference >(parent);

            if (values.size() == 0)
            {
                return {true, true, vm_expr_load_literal{"0", int_type}};
            }

            if (values.size() != 1)
            {
                throw std::runtime_error("Invalid number of arguments to integer constructor");
            }

            vm_value arg = values[0];

            qualified_symbol_reference arg_type = vm_value_type(arg);

            if (is_ref(arg_type))
            {
                arg_type = remove_ref(arg_type);
                arg = vm_expr_dereference{arg, arg_type};
            }
            if (!typeis< primitive_type_integer_reference >(arg_type))
            {
                throw std::runtime_error("Invalid argument type to integer constructor");
            }

            auto int_arg_type = boost::get< primitive_type_integer_reference >(arg_type);
            if (int_arg_type != int_type)
            {
                throw std::runtime_error("Unimplemented integer of different type passed to int constructor");
            }

            return {true, true, arg};
        }
    }

    return {true, false, {}};
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_expr(compiler* c, vm_generation_frame_info& frame, vm_block& block, vm_value callee, std::vector< vm_value > values)
{
    // TODO: support overloaded operator() of non-functions
    if (!typeis< vm_expr_bound_value >(callee))
    {
        throw std::runtime_error("Cannot call non-function reference");
    }

    vm_expr_bound_value callee_binding_value = boost::get< vm_expr_bound_value >(callee);

    vm_value callee_value = callee_binding_value.value;
    qualified_symbol_reference callee_func = callee_binding_value.function_ref;

    assert(is_canonical(callee_func));

    // TODO: Consider omitting the callee if size of type is 0
    if (!typeis< void_value >(callee_value))
    {
        values.insert(values.begin(), callee_value);
    }

    return gen_call(c, frame, block, callee_func, values);
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{

    auto tuple = try_gen_builtin_call(c, frame, block, callee, call_args);
    if (!std::get< 0 >(tuple))
    {
        return {false, {}};
    }

    if (std::get< 1 >(tuple))
    {
        return {true, std::get< 2 >(tuple)};
    }

    call_parameter_information call_set;
    call_set.argument_types = {};

    for (vm_value const& val : call_args)
    {
        call_set.argument_types.push_back(vm_value_type(val));
    }
    // TODO: Check if function parameter set already specified.

    auto overload_dp = get_dependency(
        [&]
        {
            return c->lk_function_overload_selection(callee, call_set);
        });

    if (!ready())
    {
        return {false, {}};
    }

    call_parameter_information overload = overload_dp->get();

    functanoid_reference overload_selected_ref;
    overload_selected_ref.callee = callee;
    overload_selected_ref.parameters = overload.argument_types;

    return gen_call_functanoid(c, frame, block, overload_selected_ref, call_args);
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_default_constructor(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::qualified_symbol_reference type, std::vector< vm_value > values)
{
    // TODO: make default constructing references an error

    assert(!is_ref(type));

    if (values.size() != 1)
    {
        throw std::runtime_error("Invalid number of arguments to default constructor");
    }

    auto arg_type = vm_value_type(values.at(0));
    assert(arg_type == make_mref(type) || arg_type == make_oref(type));

    if (is_ptr(type))
    {
        vm_expr_store set_nullptr;
        set_nullptr.type = type;
        set_nullptr.where = values.at(0);
        set_nullptr.what = vm_expr_load_literal{"NULLPTR", type};
        block.code.push_back(vm_execute_expression{set_nullptr});

        return {true, void_value{}};
    }

    auto layout_dp = get_dependency(
        [&]
        {
            return c->lk_class_layout_from_canonical_chain(type);
        });

    if (!ready())
    {
        return {false, {}};
    }

    class_layout layout = layout_dp->get();

    auto this_obj = values.at(0);

    for (auto const& field : layout.fields)
    {
        vm_expr_access_field access;
        access.type = make_mref(field.type);
        access.base = this_obj;
        access.offset = field.offset;

        auto field_constructor = subdotentity_reference{field.type, "CONSTRUCTOR"};

        auto pair = gen_call(c, frame, block, field_constructor, {access});
        if (!pair.first)
        {
            return {false, {}};
        }
    }

    return {true, void_value()};
}
bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::function_while_statement statement)
{
    vm_block expr_block;
    vm_while whl_statement;

    if (auto pair = gen_value_generic(c, frame, expr_block, statement.condition); pair.first)
    {
        whl_statement.condition = pair.second;
        if (expr_block.code.size() > 0)
        {
            whl_statement.condition_block = std::move(expr_block);
        }
        block.code.push_back(std::move(expr_block));
        for (auto& i : statement.loop_block.statements)
        {
            if (!build_generic(c, frame, whl_statement.loop_block, i))
            {
                return false;
            }
        }
        block.code.push_back(whl_statement);
        return true;
    }
    else
    {
        return false;
    }
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::numeric_literal expr)
{
    return {true, vm_expr_literal{expr.value}};
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_functanoid(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{


    functanoid_reference const& overload_selected_ref = boost::get< functanoid_reference >(callee);
    std::string overload_string = to_string(callee) + "  " + to_string(overload_selected_ref);

    auto arg_pair = gen_preinvoke_conversions(c, frame, block, std::move(call_args), overload_selected_ref.parameters);
    if (!arg_pair.first)
    {
        return {false, {}};
    }

    auto triple = try_gen_call_functanoid_builtin(c, frame, block, callee, arg_pair.second);

    if (!std::get< 0 >(triple))
    {
        return {false, {}};
    }

    if (std::get< 1 >(triple))
    {
        return {true, std::get< 2 >(triple)};
    }

    return gen_invoke(c, frame, block, overload_selected_ref, std::move(arg_pair.second));
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_invoke(compiler* c, vm_generation_frame_info& frame, vm_block& block, functanoid_reference const& overload_selected_ref, std::vector< vm_value > call_args)
{
    vm_expr_call call;

    call.mangled_procedure_name = mangle(overload_selected_ref);
    call.functanoid = overload_selected_ref;

    call.interface = vm_procedure_interface{};
    call.interface.argument_types = overload_selected_ref.parameters;

    // TODO: lookup return type and interface
    call.interface.return_type = primitive_type_integer_reference{32, true};

    call.arguments = std::move(call_args);

    assert(call.mangled_procedure_name != "");
    return {true, call};
}

std::pair< bool, std::vector< rylang::vm_value > > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_preinvoke_conversions(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, std::vector< vm_value > values, std::vector< qualified_symbol_reference > const& to_types)
{
    // TODO: Add support for default parameters.
    std::vector< vm_value > result;
    assert(values.size() == to_types.size());

    for (std::size_t i = 0; i < values.size(); i++)
    {
        auto pair = gen_implicit_conversion(c, frame, block, values.at(i), to_types.at(i));
        if (!pair.first)
        {
            return {false, {}};
        }
        result.push_back(pair.second);
    }
    return {true, result};
}
std::tuple< bool, bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::try_gen_call_functanoid_builtin(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::qualified_symbol_reference callee_set, std::vector< vm_value > values)
{
    assert(typeis< functanoid_reference >(callee_set));

    auto callee = boost::get< functanoid_reference >(callee_set).callee;

    if (typeis< subdotentity_reference >(callee))
    {
        subdotentity_reference const& subdot = boost::get< subdotentity_reference >(callee);
        qualified_symbol_reference parent_type = subdot.parent;

        assert(!values.empty());

        if (subdot.subdotentity_name.starts_with("OPERATOR") && typeis< primitive_type_integer_reference >(parent_type))
        {
            primitive_type_integer_reference const& int_type = boost::get< primitive_type_integer_reference >(parent_type);

            if (values.size() != 2)
            {
                throw std::runtime_error("Invalid number of arguments to integer operator");
            }

            vm_value lhs = values[0];
            vm_value rhs = values[1];

            bool is_rhs = false;

            std::string operator_str = subdot.subdotentity_name.substr(8);
            if (operator_str.ends_with("RHS"))
            {
                is_rhs = true;
                operator_str = operator_str.substr(0, operator_str.size() - 3);
                std::swap(lhs, rhs);
            }

            qualified_symbol_reference lhs_type = vm_value_type(lhs);
            qualified_symbol_reference rhs_type = vm_value_type(rhs);

            if (!assignment_operators.contains(operator_str))
            {
                assert(typeis< primitive_type_integer_reference >(lhs_type));
            }
            else
            {
              assert(typeis< primitive_type_integer_reference >(remove_ref(lhs_type)));
            }

            assert(typeis< primitive_type_integer_reference >(rhs_type));

            vm_expr_primitive_binary_op op;
            op.oper = operator_str;
            op.type = int_type;
            op.lhs = lhs;
            op.rhs = rhs;

            return {true, true, op};
        }
        if (subdot.subdotentity_name == "CONSTRUCTOR" && typeis< primitive_type_integer_reference >(parent_type))
        {
            primitive_type_integer_reference const& int_type = boost::get< primitive_type_integer_reference >(parent_type);

            // Can't call this... not possible
            if (values.empty())
            {
                throw std::runtime_error("Cannot call member function with no parameters (requires at least 'this' parameter)");
            }

            if (values.size() > 2)
            {
                throw std::runtime_error("Invalid number of arguments to integer constructor");
            }

            vm_value arg = values[0];

            qualified_symbol_reference arg_type = vm_value_type(arg);

            if (!typeis< primitive_type_integer_reference >(arg_type))
            {
                throw std::runtime_error("Invalid argument type to integer constructor");
            }

            auto int_arg_type = boost::get< primitive_type_integer_reference >(arg_type);
            if (int_arg_type != int_type)
            {
                throw std::runtime_error("Unimplemented integer of different type passed to int constructor");
            }

            if (values.size() == 1)
            {
                // default constructor
                vm_expr_store initalizer;

                initalizer.what = vm_expr_load_literal{"0", int_type};
                initalizer.where = arg;
                initalizer.type = int_type;
                block.code.push_back(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }

            else if (values.size() == 2)
            {
                // copy constructor

                vm_expr_store initalizer;

                vm_value arg_to_copy = values.at(1);
                qualified_symbol_reference arg_copy_type = vm_value_type(arg_to_copy);
                // TODO: conversion to integer?
                if (arg_copy_type != arg_type)
                {
                    throw std::runtime_error("Unimplemented integer of different type passed to int constructor");
                }

                initalizer.what = arg_to_copy;
                initalizer.where = arg;
                initalizer.type = int_type;
                block.code.push_back(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }
        }
        else if (subdot.subdotentity_name == "CONSTRUCTOR")
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen_dp = get_dependency(
                [&]
                {
                    return c->lk_class_should_autogen_default_constructor(parent_type);
                });
            if (!ready())
            {
                return {false, false, {}};
            }
            bool should_autogen = should_autogen_dp->get();
            if (!should_autogen)
            {
                return {true, false, {}};
            }

            auto pair = gen_default_constructor(c, frame, block, parent_type, values);
            if (pair.first)
            {
                return {true, true, pair.second};
            }
            else
            {
                return {false, {}, {}};
            }
        }
    }

    return {true, false, {}};
}

rylang::vm_value rylang::vm_procedure_from_canonical_functanoid_resolver::gen_conversion_to_integer(rylang::compiler* c, rylang::vm_generation_frame_info& frame, rylang::vm_block& block, rylang::vm_expr_literal val, rylang::primitive_type_integer_reference to_type)
{
    vm_expr_load_literal result = vm_expr_load_literal{val.literal, to_type};

    return result;
}
