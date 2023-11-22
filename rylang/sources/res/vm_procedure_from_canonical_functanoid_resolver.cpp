//
// Created by Ryan Nicholl on 11/5/23.
//

#include "rylang/compiler.hpp"

#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"

#include "rylang/data/vm_generation_frameinfo.hpp"
#include "rylang/manipulators/mangler.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "rylang/operators.hpp"
#include "rylang/variant_utils.hpp"

#include <exception>

// TODO: Debugging, remove this
#include <iostream>

namespace rylang
{

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver* res, qualified_symbol_reference func, class compiler* c, vm_generation_frame_info& frame, vm_block& block)
        : m_c(c)
        , m_ctx(func)
        , m_frame(frame)
        , m_block(block)
        , m_resolver(res)
    {
        m_insertion_point = [this](vm_block b)
        {
            m_block.code.emplace_back(std::move(b));
        };
        exception_ct = std::uncaught_exceptions();
        assert(m_frame.blocks.empty());

        m_frame.blocks.emplace_back();
        m_frame.blocks.back().context_overload = func;
        m_frame.context = func;
    }

    vm_procedure_from_canonical_functanoid_resolver::context_frame::~context_frame() noexcept(false)
    {
        if (std::uncaught_exceptions() != exception_ct)
        {
            return;
        }
        if (!closed)
        {
            throw std::logic_error("Context frame destroyed without being closed or discarded");
        }
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other)
        : m_c(other.m_c)
        , m_frame(other.m_frame)
        , m_block(other.m_block)
        , m_resolver(other.m_resolver)
    {
        m_frame.blocks.emplace_back(m_frame.blocks.back());
        for (auto& var : m_frame.blocks.back().value_states)
        {
            var.second.this_frame = false;
        }
        m_insertion_point = [this](vm_block b)
        {
            m_block.code.emplace_back(std::move(b));
        };
    }

    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::close()
    {
        for (auto& var : m_frame.blocks.back().value_states)
        {
            if (var.second.this_frame && var.second.alive)
            {
                if (!destroy_value(var.first))
                {
                    return false;
                }
            }
        }
        assert(m_insertion_point);
        m_insertion_point(std::move(m_new_block));
        return true;
    }
    void vm_procedure_from_canonical_functanoid_resolver::context_frame::discard()
    {
        closed = true;
    }

    std::pair< bool, std::optional< vm_value > > vm_procedure_from_canonical_functanoid_resolver::context_frame::try_load_variable(std::string name)
    {
        assert(!(m_frame.blocks.empty()));

        vm_generation_block& current_block = m_frame.blocks.back();

        auto pos = current_block.variable_lookup_index.find(name);
        if (pos == current_block.variable_lookup_index.end())
        {
            return {true, std::nullopt};
        }
        auto index = pos->second;
        vm_expr_load_address load;
        load.index = index;
        auto vartype = m_frame.variables.at(index).type;

        std::string vartype_str = to_string(vartype);

        assert(is_canonical(vartype));

        if (is_ref(vartype))
        {
            load.type = vartype;
        }
        else
        {
            load.type = make_mref(vartype);
        }
        std::string loadtype_str = to_string(load.type);

        return {true, load};
    }

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::condition_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.condition_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::then_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.then_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::else_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            if (!b.code.empty())

            {
                insertion_point.else_block = std::move(b);
            }
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other, vm_while& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::condition_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.condition_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame const& other, vm_while& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::loop_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.loop_block = std::move(b);
        };
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::load_variable(std::string name)
    {
        auto [success, val] = try_load_variable(name);
        if (!success)
        {
            return {false, {}};
        }
        // TODO: should this be an assert?
        assert(val.has_value());
        return {success, val.value()};
    }
    qualified_symbol_reference vm_procedure_from_canonical_functanoid_resolver::context_frame::current_context() const
    {
        return m_frame.context;
    }
    std::pair< bool, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_value_storage(std::optional< std::string > name, qualified_symbol_reference type)
    {
        bool temp = !(name.has_value());

        std::string var_type_str = to_string(type);
        compiler::out< type_placement_info > type_placement_info_dp = m_resolver->get_dependency(
            [&]
            {
                return compiler()->lk_type_placement_info_from_canonical_type(type);
            });
        if (!m_resolver->ready())
        {
            return {false, {}};
        }
        type_placement_info type_placement_info_v = type_placement_info_dp->get();
        {
            vm_allocate_storage storage;
            storage.size = type_placement_info_v.size;
            storage.align = type_placement_info_v.alignment;
            push(storage);
        }
        vm_frame_variable var;
        var.name = name.value_or("TEMPORARY");
        var.type = type;
        var.is_temporary = temp;
        vm_expr_load_address load;
        if (temp)
        {
            load.type = make_tref(type);
        }
        else
        {
            load.type = make_mref(type);
        }
        load.index = m_frame.variables.size();
        var.get_addr = load;
        m_frame.variables.push_back(var);
        if (name.has_value())
        {
            m_frame.blocks.back().variable_lookup_index[*name] = m_frame.variables.size() - 1;
        }
        m_frame.blocks.back().value_states[m_frame.variables.size() - 1].alive = false;
        m_frame.blocks.back().value_states[m_frame.variables.size() - 1].this_frame = true;

        return {true, m_frame.variables.size() - 1};
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::load_temporary(std::size_t index)
    {
        return load_value(index, true, true);
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::load_temporary_as_new(std::size_t index)
    {
        return load_value(index, false, true);
    }

    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::construct_new_variable(std::string name, qualified_symbol_reference type, std::vector< vm_value > args)
    {

        auto [ok, index] = create_variable_storage(name, type);
        // TODO: Set destructor here if needed.

        if (!ok)
        {
            assert(!m_resolver->ready());
            return false;
        }
        auto [ok2, val] = load_variable_as_new(index);
        if (!ok2)
        {
            assert(!m_resolver->ready());
            return false;
        }

        auto ok3 = run_value_constructor(index, args);
        if (!ok3)
        {
            assert(!m_resolver->ready());
            return false;
        }

        auto ok4 = set_value_alive(index);
        if (!ok4)
        {
            assert(!m_resolver->ready());
            return false;
        }

        return true;
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value(std::size_t index, bool alive, bool temp)
    {
        vm_expr_load_address load;
        load.index = index;
        auto vartype = m_frame.variables.at(index).type;

        assert(m_frame.variables.at(index).is_temporary == temp);
        assert(m_frame.blocks.back().value_states.at(index).alive == alive);

        std::string vartype_str = to_string(vartype);

        assert(is_canonical(vartype));

        if (is_ref(vartype))
        {
            load.type = vartype;
        }
        else
        {
            if (!alive || !temp)
            {
                load.type = make_mref(vartype);
            }
            else
            {
                load.type = make_tref(vartype);
            }
        }
        std::string loadtype_str = to_string(load.type);

        return {true, load};
    }

    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::set_value_alive(std::size_t index)
    {
        m_frame.blocks.back().value_states.at(index).alive = true;
        return true;
    }
    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::set_value_dead(std::size_t index)
    {
        m_frame.blocks.back().value_states.at(index).alive = false;
        return true;
    }
    std::pair< bool, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_variable_storage(std::string name, qualified_symbol_reference type)
    {
        return create_value_storage(name, type);
    }

    std::pair< bool, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_temporary_storage(qualified_symbol_reference type)
    {
        return create_value_storage(std::nullopt, type);
    }
    std::pair< bool, qualified_symbol_reference > vm_procedure_from_canonical_functanoid_resolver::context_frame::get_variable_type(std::size_t index)
    {
        return {true, m_frame.variables.at(index).type};
    }
    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::frame_return(vm_value val)
    {
        if (!set_return_value(val))
        {
            return false;
        }
        return frame_return();
    }
    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::frame_return()
    {
        for (std::size_t i; i < m_frame.variables.size(); i++)
        {
            if (m_frame.blocks.back().value_states.at(i).alive)
            {
                if (!destroy_value(i))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::destroy_value(std::size_t index)
    {
        auto ok = run_value_destructor(index);
        if (!ok)
        {
            return false;
        }
        auto ok2 = set_value_dead(index);
        if (!ok)
        {
            return false;
        }

        return true;
    }
    std::pair< bool, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::adopt_value_as_temporary(vm_value val)
    {
        assert(!is_ref(vm_value_type(val)));
        auto [ok, index] = create_temporary_storage(vm_value_type(val));
        if (!ok)
        {
            return {false, {}};
        }
        auto [ok2, temp_location] = load_temporary_as_new(index);
        vm_store str;
        str.type = vm_value_type(val);
        str.what = val;
        str.where = temp_location;
        push(std::move(str));
        auto ok3 = set_value_alive(index);
        if (!ok3)
        {
            return {false, {}};
        }

        return {true, index};
    }
    std::pair< bool, std::optional< std::size_t > > vm_procedure_from_canonical_functanoid_resolver::context_frame::try_get_variable_index(std::string name)
    {
        auto it = m_frame.blocks.back().variable_lookup_index.find(name);
        if (it == m_frame.blocks.back().variable_lookup_index.end())
        {
            return {true, std::nullopt};
        }
        return {true, it->second};
    }
    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::run_value_destructor(std::size_t index)
    {
        auto [ok, type] = get_variable_type(index);

        if (!ok)
        {
            return false;
        }
        auto [ok2, val] = load_value_as_desctructable(index);
        if (!ok2)
        {
            return false;
        }
        subdotentity_reference destructor_symbol = {type, "DESTRUCTOR"};

        auto [ok3, res] = m_resolver->gen_call(*this, destructor_symbol, {val});
        if (!ok3)
        {
            return false;
        }
        return true;
    }
    bool vm_procedure_from_canonical_functanoid_resolver::context_frame::run_value_constructor(std::size_t index, std::vector< vm_value > args)
    {
        auto [ok, type] = get_variable_type(index);

        if (!ok)
        {
            return false;
        }
        auto [ok2, val] = load_variable_as_new(index);
        if (!ok2)
        {
            return false;
        }
        subdotentity_reference constructor_symbol = {type, "CONSTRUCTOR"};
        args.insert(args.begin(), val);

        auto [ok3, res] = m_resolver->gen_call(*this, constructor_symbol, args);
        if (!ok3)
        {
            return false;
        }
        return true;
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value_as_desctructable(std::size_t index)
    {
        bool alive = true;
        vm_expr_load_address load;
        load.index = index;
        auto vartype = m_frame.variables.at(index).type;

        assert(m_frame.blocks.back().value_states.at(index).alive == alive);

        std::string vartype_str = to_string(vartype);

        assert(is_canonical(vartype));

        if (is_ref(vartype))
        {
            load.type = vartype;
        }
        else
        {
            load.type = make_mref(vartype);
        }
        std::string loadtype_str = to_string(load.type);

        return {true, load};
    }
    std::pair< bool, vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::set_return_value(vm_value)
    {
        return std::pair< bool, vm_value >();
    }
} // namespace rylang

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

    {
        context_frame ctx(this, m_func_name, c, frame, vm_proc.body);

        // If the function returns a value, that is the first variable

        if (function_ast_v.return_type)
        {
            vm_frame_variable var;
            var.name = "RETURN_VALUE";
            var.type = function_ast_v.return_type.value();
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index["return"] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = false;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = false;
            vm_proc.interface.return_type = function_ast_v.return_type.value();
        }

        for (auto& arg : function_ast_v.args)
        {
            // TODO: consider pass by pointer of large values instead of by value
            vm_frame_variable var;
            // NOTE: Make sure that we can handle contextual types in arg list.
            //  They should be decontextualized somewhere else probably.. so maybe this will
            //  be impossible.

            // TODO: Make sure no name conflicts are allowed.
            assert(!qualified_is_contextual(arg.type));
            var.name = arg.name;
            var.type = arg.type;
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index[arg.name] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = true;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = true;
            vm_proc.interface.argument_types.push_back(arg.type);

            // TODO: Maybe consider adding ctx.add_argument instead of doing this inside this function
        }

        // Then generate the body
        if (!build_generic(ctx, function_ast_v.body))
        {
            ctx.discard();
            return;
        }

        ctx.close();
    }
    assert(frame.blocks.empty());

    set_value(vm_proc);
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_var_statement statement)
{
    vm_allocate_storage storage;

    std::string var_type_str = to_string(statement.type);

    auto type_canonical_dp = get_dependency(
        [&]
        {
            return ctx.compiler()->lk_canonical_type_from_contextual_type(statement.type, ctx.current_context());
        });
    if (!ready())
    {
        return false;
    }
    qualified_symbol_reference canonical_type = type_canonical_dp->get();

    std::string canonical_var_str = to_string(canonical_type);

    return ctx.construct_new_variable(statement.name, canonical_type, {});
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_expression_statement statement)
{

    context_frame expr_ctx(ctx);

    // Each expression needs to have its own block to store temporary lvalues,
    // e.g. a := b + c, creates a temporary lvalue for the result of "b + c" before executing
    // the assignment itself.
    if (auto pair = gen_value_generic(expr_ctx, statement.expr); pair.first)
    {
        vm_execute_expression exc;
        exc.expr = pair.second;
        expr_ctx.push(std::move(exc));
        return expr_ctx.close();
    }
    else
    {
        expr_ctx.discard();
        return false;
    }
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_generic(context_frame& ctx, expression expr)
{
    if (typeis< expression_lvalue_reference >(expr))
    {
        assert(false);
    }
    if (typeis< expression_symbol_reference >(expr))
    {
        auto res = gen_value(ctx, boost::get< expression_symbol_reference >(std::move(expr)));
        assert(res.first == ready());
        return res;
    }
    else if (typeis< expression_copy_assign >(expr))
    {
        auto res = gen_value(ctx, boost::get< expression_copy_assign >(std::move(expr)));
        assert(res.first == ready());
        return res;
    }
    else if (typeis< expression_binary >(expr))
    {
        auto res = gen_value(ctx, boost::get< expression_binary >(std::move(expr)));
        assert(res.first == ready());
        return res;
    }
    else if (typeis< expression_call >(expr))
    {
        auto res = gen_value(ctx, boost::get< expression_call >(std::move(expr)));
        assert(res.first == ready());
        return res;
    }
    else if (typeis< numeric_literal >(expr))
    {
        auto res = gen_value(ctx, boost::get< numeric_literal >(std::move(expr)));
        assert(res.first == ready());
        return res;
    }
    else
    {
        // TODO: Add other types
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }
    // gen_expression(ctx, expr);
    assert(false);
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_lvalue_reference expr)
{

    std::string name = expr.identifier;
    auto [ok, value] = ctx.try_get_variable_index(name);

    if (!ok)
    {
        return {false, {}};
    }

    if (!value.has_value())
    {
        throw std::logic_error("Couldn't find lvalue in frame");
    }

    auto [ok2, result] = ctx.load_variable(value.value());

    return {ok2, result};
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_copy_assign expr)
{
    // TODO: Support overloading

    auto lhs_pair = gen_value_generic(ctx, expr.lhs);
    auto rhs_pair = gen_value_generic(ctx, expr.rhs);

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

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_return_statement statement)
{

    if (statement.expr.has_value())
    {
        context_frame return_ctx(ctx);
        auto pair = gen_value_generic(return_ctx, statement.expr.value());
        if (!pair.first)
        {
            return_ctx.discard();
            assert(!ready());
            return false;
        }
        auto retval = pair.second;
        auto vtype = vm_value_type(retval);
        // TODO: Support returning references?
        if (is_ref(vtype))
        {
            vtype = remove_ref(vtype);
            retval = vm_expr_dereference{retval, vtype};
        }
        auto ok = return_ctx.frame_return(std::move(retval));
        if (!ok)
        {
            return_ctx.discard();
            assert(!ready());
            return false;
        }
        return return_ctx.close();
    }
    return ctx.frame_return();
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_if_statement statement)
{
    vm_if if_stmt;
    context_frame if_condition_ctx(ctx, if_stmt, context_frame::condition_tag);
    auto [ok, condvalue] = gen_value_generic(if_condition_ctx, statement.condition);
    if (!ok)
    {
        if_condition_ctx.discard();
        return false;
    }
    if_stmt.condition = std::move(condvalue);
    ok = if_condition_ctx.close();
    if (!ok)
    {
        return false;
    }
    context_frame if_then_ctx(ctx, if_stmt, context_frame::then_tag);
    if (!build_generic(if_then_ctx, statement.then_block))
    {
        if_then_ctx.discard();
        return false;
    }
    ok = if_then_ctx.close();
    if (!ok)
    {
        return false;
    }
    if (statement.else_block.has_value())
    {
        context_frame if_else_ctx(ctx, if_stmt, context_frame::else_tag);
        if (!build_generic(if_else_ctx, statement.else_block.value()))
        {
            if_else_ctx.discard();
            return false;
        }
        ok = if_else_ctx.close();
        if (!ok)
        {
            return false;
        }
    }
    ctx.push(std::move(if_stmt));
    return true;
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_binary expr)
{
    // TODO: Support overloading

    auto lhs_pair = gen_value_generic(ctx, expr.lhs);
    auto rhs_pair = gen_value_generic(ctx, expr.rhs);

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
            return ctx.compiler()->lk_functum_exists_and_is_callable_with(lhs_function, lhs_param_info);
        });

    if (!ready())
    {
        return {false, {}};
    }

    bool lhs_exists_and_callable_with = lhs_exists_and_callable_with_dp->get();

    if (lhs_exists_and_callable_with)
    {
        return gen_call(ctx, lhs_function, std::vector< vm_value >{lhs, rhs});
    }

    auto rhs_exists_and_callable_with_dp = get_dependency(
        [&]
        {
            return ctx.compiler()->lk_functum_exists_and_is_callable_with(rhs_function, rhs_param_info);
        });

    if (!ready())
    {
        return {false, {}};
    }

    bool rhs_exists_and_callable_with = rhs_exists_and_callable_with_dp->get();

    if (rhs_exists_and_callable_with)
    {
        return gen_call(ctx, rhs_function, std::vector< vm_value >{rhs, lhs});
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_call expr)
{
    auto callee_pv = gen_value_generic(ctx, expr.callee);
    bool ok = callee_pv.first;

    if (!ok)
    {
        assert(!ready());
        return {false, {}};
    }

    std::vector< vm_value > args;

    for (auto& arg : expr.args)
    {
        // TODO: Translate arg types from references

        auto pv = gen_value_generic(ctx, arg);
        ok = pv.first;
        if (!ok)
        {
            assert(!ready());
            return {false, {}};
        }
        args.push_back(pv.second);
    }

    auto res = gen_call_expr(ctx, callee_pv.second, args);
    assert(res.first == ready());
    return res;
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_symbol_reference expr)
{
    bool is_possibly_frame_value = typeis< subentity_reference >(expr.symbol) && typeis< context_reference >(boost::get< subentity_reference >(expr.symbol).parent);

    // Frame values are sub-entities of the current context.
    if (is_possibly_frame_value)
    {
        std::string name = boost::get< subentity_reference >(expr.symbol).subentity_name;

        auto pair = ctx.try_load_variable(name);
        if (!pair.first)
        {
            return {false, {}};
        }
        if (pair.second.has_value())
        {
            return {true, pair.second.value()};
        }
    }

    // assert(ctx.current_context() == m_func_name);
    auto canonical_symbol_dp = get_dependency(
        [&]
        {
            return ctx.compiler()->lk_canonical_type_from_contextual_type(expr.symbol, m_func_name);
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
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_implicit_conversion(context_frame& ctx, rylang::vm_value from, rylang::qualified_symbol_reference to)
{
    auto from_type = vm_value_type(from);

    if (from_type == to)
    {
        return {true, from};
    }

    if (remove_ref(from_type) == to)
    {
        return gen_ref_to_value(ctx, from);
    }

    else if (from_type == remove_ref(to))
    {
        return gen_value_to_ref(ctx, from, to);
    }

    if (is_ref(from_type) && is_ref(to) && remove_ref(from_type) == remove_ref(to))
    {
        return {true, vm_expr_reinterpret{from, to}};
    }

    auto underlying_to_type = remove_ref(to);
    if (typeis< primitive_type_integer_reference >(to) && typeis< vm_expr_literal >(from))
    {
        auto result = gen_conversion_to_integer(ctx, boost::get< vm_expr_literal >(from), boost::get< primitive_type_integer_reference >(to));

        if (is_ref(to))
        {
            return gen_value_to_ref(ctx, result, to);
        }
        else
        {
            return {true, result};
        }
    }

    // TODO: Allowed integer conversions, etc

    throw std::runtime_error("Cannot convert between these types");
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_to_ref(context_frame& ctx, rylang::vm_value from, rylang::qualified_symbol_reference to_type)
{

    assert(is_canonical(to_type));

    auto [ok, index] = ctx.adopt_value_as_temporary(from);

    return ctx.load_temporary(index);
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_ref_to_value(context_frame& ctx, rylang::vm_value val)
{
    auto arg_type = vm_value_type(val);
    qualified_symbol_reference to_type = remove_ref(arg_type);

    assert(is_canonical(arg_type));
    // TODO: Copy constructor, if needed, etc.
    return {true, vm_expr_dereference{val, to_type}};
}

std::tuple< bool, bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::try_gen_builtin_call(context_frame& ctx, rylang::qualified_symbol_reference callee, std::vector< vm_value > values)
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
                ctx.push(vm_execute_expression{initalizer});
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
                ctx.push(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }
        }
        else if (subdot.subdotentity_name == "CONSTRUCTOR")
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen_dp = get_dependency(
                [&]
                {
                    return ctx.compiler()->lk_class_should_autogen_default_constructor(parent_type);
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

            auto pair = gen_default_constructor(ctx, parent_type, values);
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
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_expr(context_frame& ctx, vm_value callee, std::vector< vm_value > values)
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

    auto res = gen_call(ctx, callee_func, values);
    assert(res.first == ready());
    return res;
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call(context_frame& ctx, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{

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
            return ctx.compiler()->lk_function_overload_selection(callee, call_set);
        });

    if (!ready())
    {
        return {false, {}};
    }

    call_parameter_information overload = overload_dp->get();

    functanoid_reference overload_selected_ref;
    overload_selected_ref.callee = callee;
    overload_selected_ref.parameters = overload.argument_types;

    return gen_call_functanoid(ctx, overload_selected_ref, call_args);
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_default_constructor(context_frame& ctx, rylang::qualified_symbol_reference type, std::vector< vm_value > values)
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
        ctx.push(vm_execute_expression{set_nullptr});

        return {true, void_value{}};
    }

    auto layout_dp = get_dependency(
        [&]
        {
            return ctx.compiler()->lk_class_layout_from_canonical_chain(type);
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

        auto pair = gen_call(ctx, field_constructor, {access});
        if (!pair.first)
        {
            return {false, {}};
        }
    }

    return {true, void_value()};
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_while_statement statement)
{
    vm_while whl_statement;
    context_frame while_condition_frame(ctx, whl_statement, context_frame::condition_tag);
    auto pair = gen_value_generic(while_condition_frame, statement.condition);
    if (!pair.first)
    {
        while_condition_frame.discard();
        return false;
    }
    auto ok = while_condition_frame.close();
    if (!ok)
    {
        return false;
    }
    whl_statement.condition = pair.second;
    context_frame while_loop_ctx(ctx, whl_statement, context_frame::loop_tag);
    for (auto& i : statement.loop_block.statements)
    {
        if (!build_generic(while_loop_ctx, i))
        {
            while_loop_ctx.discard();
            return false;
        }
    }
    return while_loop_ctx.close();
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::numeric_literal expr)
{
    return {true, vm_expr_literal{expr.value}};
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_functanoid(context_frame& ctx, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{

    functanoid_reference const& overload_selected_ref = boost::get< functanoid_reference >(callee);
    std::string overload_string = to_string(callee) + "  " + to_string(overload_selected_ref);

    auto arg_pair = gen_preinvoke_conversions(ctx, std::move(call_args), overload_selected_ref.parameters);
    if (!arg_pair.first)
    {
        assert(!ready());
        return {false, {}};
    }

    auto triple = try_gen_call_functanoid_builtin(ctx, callee, arg_pair.second);

    if (!std::get< 0 >(triple))
    {
        assert(!ready());
        return {false, {}};
    }

    if (std::get< 1 >(triple))
    {
        return {true, std::get< 2 >(triple)};
    }

    auto res = gen_invoke(ctx, overload_selected_ref, std::move(arg_pair.second));
    assert(res.first == ready());
    return res;
}

std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_invoke(context_frame& ctx, functanoid_reference const& overload_selected_ref, std::vector< vm_value > call_args)
{
    vm_expr_call call;

    std::string typestr = to_string(overload_selected_ref);

    call.mangled_procedure_name = mangle(overload_selected_ref);
    call.functanoid = overload_selected_ref;

    call.interface = vm_procedure_interface{};
    call.interface.argument_types = overload_selected_ref.parameters;

    // TODO: lookup return type and interface
    call.interface.return_type = primitive_type_integer_reference{32, true};

    call.arguments = std::move(call_args);

    assert(call.mangled_procedure_name != "");
    if (call.interface.return_type.has_value())
    {
        auto [ok, temp_index] = ctx.create_temporary_storage(call.interface.return_type.value());
        if (!ok)
        {
            return {false, {}};
        }

        auto [ok2, tempval] = ctx.load_temporary_as_new(temp_index);
        if (!ok2)
        {
            return {false, {}};
        }

        vm_store call_result;
        call_result.type = call.interface.return_type.value();
        call_result.where = tempval;
        call_result.what = call;

        ctx.push(call_result);

        auto ok3 = ctx.set_value_alive(temp_index);
        if (!ok3)
        {
            return {false, {}};
        }
    }
    return {true, call};
}

std::pair< bool, std::vector< rylang::vm_value > > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_preinvoke_conversions(context_frame& ctx, std::vector< vm_value > values, std::vector< qualified_symbol_reference > const& to_types)
{
    // TODO: Add support for default parameters.
    std::vector< vm_value > result;
    assert(values.size() == to_types.size());

    for (std::size_t i = 0; i < values.size(); i++)
    {
        auto pair = gen_implicit_conversion(ctx, values.at(i), to_types.at(i));
        if (!pair.first)
        {
            return {false, {}};
        }
        result.push_back(pair.second);
    }
    return {true, result};
}
std::tuple< bool, bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::try_gen_call_functanoid_builtin(context_frame& ctx, rylang::qualified_symbol_reference callee_set, std::vector< vm_value > values)
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

            if (!typeis< mvalue_reference >(arg_type) || !typeis< primitive_type_integer_reference >(remove_ref(arg_type)))
            {
                throw std::runtime_error("Invalid argument type to integer constructor");
            }

            auto int_arg_type = boost::get< primitive_type_integer_reference >(remove_ref(arg_type));
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
                ctx.push(vm_execute_expression{initalizer});
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
                ctx.push(vm_execute_expression{initalizer});
                return {true, true, void_value{}};
            }
        }
        else if (subdot.subdotentity_name == "CONSTRUCTOR")
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen_dp = get_dependency(
                [&]
                {
                    return ctx.compiler()->lk_class_should_autogen_default_constructor(parent_type);
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

            auto pair = gen_default_constructor(ctx, parent_type, values);
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

rylang::vm_value rylang::vm_procedure_from_canonical_functanoid_resolver::gen_conversion_to_integer(context_frame& ctx, rylang::vm_expr_literal val, rylang::primitive_type_integer_reference to_type)
{
    vm_expr_load_literal result = vm_expr_load_literal{val.literal, to_type};

    return result;
}

bool rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_block statement)
{
    context_frame block_frame(ctx);
    for (auto& i : statement.statements)
    {
        if (!build_generic(block_frame, i))
        {
            block_frame.discard();
            assert(!ready());
            return false;
        }
        // TODO: Maybe consider not doing this later for adding GOTO statements.
        if (typeis< function_return_statement >(i))
        {
            break;
        }
    }
    return block_frame.close();
}
std::pair< bool, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_default_destructor(context_frame& ctx, rylang::qualified_symbol_reference type, std::vector< vm_value > values)
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
        ctx.push(vm_execute_expression{set_nullptr});

        return {true, void_value{}};
    }

    auto layout_dp = get_dependency(
        [&]
        {
            return ctx.compiler()->lk_class_layout_from_canonical_chain(type);
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

        auto field_destructor = subdotentity_reference{field.type, "DESTRUCTOR"};

        auto pair = gen_call(ctx, field_destructor, {access});
        if (!pair.first)
        {
            return {false, {}};
        }
    }

    return {true, void_value()};
}
bool rylang::vm_procedure_from_canonical_functanoid_resolver::build_generic(rylang::vm_procedure_from_canonical_functanoid_resolver::context_frame& ctx, rylang::function_statement statement)
{

    if (typeis< function_var_statement >(statement))
    {
        function_var_statement var_stmt = boost::get< function_var_statement >(statement);
        auto res = build(ctx, var_stmt);
        assert(res == ready());
        return res;
    }
    else if (typeis< function_expression_statement >(statement))
    {
        function_expression_statement expr_stmt = boost::get< function_expression_statement >(statement);
        auto res = build(ctx, expr_stmt);
        assert(res == ready());
        return res;
    }
    else if (typeis< function_if_statement >(statement))
    {
        function_if_statement if_stmt = boost::get< function_if_statement >(statement);
        auto res = build(ctx, if_stmt);
        assert(res == ready());
        return res;
    }
    else if (typeis< function_while_statement >(statement))
    {
        function_while_statement while_stmt = boost::get< function_while_statement >(statement);

        auto res = build(ctx, while_stmt);
        assert(res == ready());
        return res;
    }
    else if (typeis< function_return_statement >(statement))
    {
        function_return_statement return_stmt = boost::get< function_return_statement >(statement);
        auto res = build(ctx, return_stmt);
        assert(res == ready());
        return res;
    }
    else if (typeis< function_block >(statement))
    {
        function_block block_stmt = boost::get< function_block >(statement);
        auto res = build(ctx, block_stmt);
        assert(res == ready());
        return res;
    }
    else
    {
        throw std::runtime_error("Unknown function statement type");
    }
    throw std::runtime_error("unimplemented");
}
