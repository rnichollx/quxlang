//
// Created by Ryan Nicholl on 11/5/23.
//

#include "rylang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"

#include "rylang/data/vm_generation_frameinfo.hpp"
#include "rylang/manipulators/mangler.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "rylang/operators.hpp"
#include "rylang/variant_utils.hpp"

#include <exception>

// TODO: Debugging, remove this
#include "rylang/to_pretty_string.hpp"

#include <iostream>

using namespace rylang;

namespace rylang
{

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver* res, qualified_symbol_reference func, class compiler* c, vm_generation_frame_info& frame, vm_procedure& proc)
        : m_c(c)
        , m_ctx(func)
        , m_frame(frame)
        , m_resolver(res)
        , m_proc(proc)
    {
        vm_block& block = proc.body;
       // std::cout << "Enter context frame" << std::endl;
        m_insertion_point = [this, &block](vm_block b)
        {
            block.code.emplace_back(std::move(b));
        };
        exception_ct = std::uncaught_exceptions();
        assert(m_frame.blocks.empty());

        m_frame.blocks.emplace_back();
        m_frame.blocks.back().context_overload = func;
        m_frame.context = func;
        comment("initial context frame");
    }

    vm_procedure_from_canonical_functanoid_resolver::context_frame::~context_frame() noexcept(false)
    {
        // std::cout << "context frame destructor" << std::endl;
        if (std::uncaught_exceptions() != exception_ct)
        {
            std::cout << "Exception return from context frame" << std::endl;
            return;
        }
        if (!closed)
        {
            // std::cout << "improperly destroyed context frame" << std::endl;
            throw std::logic_error("Context frame destroyed without being closed or discarded");
        }
    }

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(context_frame& other)
        : m_c(other.m_c)
        , m_frame(other.m_frame)
        , m_resolver(other.m_resolver)
        , m_proc(other.m_proc)
    {
        assert(m_frame.blocks.back().value_states[0].alive == false);

        // std::cout << "Enter duplicate context" << std::endl;
        m_frame.blocks.emplace_back(m_frame.blocks.back());
        for (auto& var : m_frame.blocks.back().value_states)
        {
            var.second.this_frame = false;
        }
        auto ptr = this;
        m_insertion_point = [ptr, &other](vm_block b)
        {
            other.m_new_block.code.emplace_back(std::move(b));
        };
    }

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::close()
    {
        if (closed)
        {
            throw std::logic_error("Already closed");
        }
        for (auto& var : m_frame.blocks.back().value_states)
        {
            if (var.second.this_frame && var.second.alive)
            {
                co_await destroy_value(var.first);
            }
        }
        assert(m_insertion_point);
        m_insertion_point(std::move(m_new_block));
        assert(m_frame.blocks.size() != 0);
        m_frame.blocks.pop_back();
        closed = true;
        co_return {};
    }
    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::discard()
    {
        std::cout << "Context discarded" << std::endl;
        closed = true;
        co_return {};
    }

    std::optional< vm_value > vm_procedure_from_canonical_functanoid_resolver::context_frame::try_load_variable(std::string name)
    {
        assert(!(m_frame.blocks.empty()));

        vm_generation_block& current_block = m_frame.blocks.back();

        auto pos = current_block.variable_lookup_index.find(name);
        if (pos == current_block.variable_lookup_index.end())
        {
            return std::nullopt;
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

        return load;
    }

    void vm_procedure_from_canonical_functanoid_resolver::context_frame::comment(std::string const& str)
    {
        m_new_block.comments.push_back(str);
    }

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::condition_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.condition_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::then_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.then_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame& other, vm_if& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::else_t)
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
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame& other, vm_while& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::condition_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.condition_block = std::move(b);
        };
    }
    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver::context_frame& other, vm_while& insertion_point, vm_procedure_from_canonical_functanoid_resolver::context_frame::loop_t)
        : context_frame(other)
    {
        m_insertion_point = [this, &insertion_point](vm_block b)
        {
            insertion_point.loop_block = std::move(b);
        };
    }
    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_variable(std::string name)
    {
        auto val = try_load_variable(name);
        assert(val.has_value());
        return val.value();
    }
    qualified_symbol_reference vm_procedure_from_canonical_functanoid_resolver::context_frame::current_context() const
    {
        return m_frame.context;
    }
    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_value_storage(std::optional< std::string > name, qualified_symbol_reference type)
    {
        bool temp = !(name.has_value());

        std::string var_type_str = to_string(type);

        type_placement_info type_placement_info_v = co_await *compiler()->lk_type_placement_info_from_canonical_type(type);

        vm_allocate_storage storage;
        storage.size = type_placement_info_v.size;
        storage.align = type_placement_info_v.alignment;
        if (name.has_value())
        {
            storage.kind = storage_type::local;
        }
        else
        {
            storage.kind = storage_type::temporary;
        }
        storage.type = type;
        vm_frame_variable var;
        var.name = name.value_or("TEMPORARY");
        var.type = type;
        var.is_temporary = temp;
        var.storage = storage;
        assert(var.storage.valid());
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

        co_return m_frame.variables.size() - 1;
    }

    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_temporary(std::size_t index)
    {
        return load_value(index, true, true);
    }
    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_temporary_as_new(std::size_t index)
    {
        return load_value(index, false, true);
    }

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::construct_new_variable(std::string name, qualified_symbol_reference type, std::vector< vm_value > args)
    {

        auto index = co_await create_variable_storage(name, type);

        auto val = load_variable_as_new(index);

        co_await run_value_constructor(index, args);

        set_value_alive(index);

        co_return {};
    }
    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value(std::size_t index, bool alive, bool temp)
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

        return load;
    }

    void vm_procedure_from_canonical_functanoid_resolver::context_frame::set_value_alive(std::size_t index)
    {
        //std::cout << "set < " << index << " > alive" << std::endl;
        if (index == 0)
        {
         //   std::cout << "set "
         //             << "return alive" << std::endl;
        }
        assert(m_frame.blocks.back().value_states[index].alive == false);
        m_frame.blocks.back().value_states.at(index).alive = true;
        return;
    }
    void vm_procedure_from_canonical_functanoid_resolver::context_frame::set_value_dead(std::size_t index)
    {
        m_frame.blocks.back().value_states.at(index).alive = false;
        return;
    }
    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_variable_storage(std::string name, qualified_symbol_reference type)
    {
        return create_value_storage(name, type);
    }



    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_temporary_storage(qualified_symbol_reference type)
    {
        return create_value_storage(std::nullopt, type);
    }
    qualified_symbol_reference vm_procedure_from_canonical_functanoid_resolver::context_frame::get_variable_type(std::size_t index)
    {
        return m_frame.variables.at(index).type;
    }
    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::frame_return(vm_value val)
    {
        co_await set_return_value(val);

        co_return co_await frame_return();
    }
    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::frame_return()
    {
        for (std::size_t i = 1; i < m_frame.variables.size(); i++)
        {
            if (m_frame.blocks.back().value_states.count(i) != 0 && m_frame.blocks.back().value_states.at(i).alive)
            {
                co_await destroy_value(i);
            }
        }

        push(vm_return{});
        co_return {};
    }

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::destroy_value(std::size_t index)
    {
        co_await run_value_destructor(index);
        set_value_dead(index);
        co_return {};
    }

    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::adopt_value_as_temporary(vm_value val)
    {
        assert(!is_ref(vm_value_type(val)));
        auto index = co_await create_temporary_storage(vm_value_type(val));

        auto temp_location = load_temporary_as_new(index);
        vm_store str;
        str.type = vm_value_type(val);
        str.what = val;
        str.where = temp_location;
        push(std::move(str));

        co_return index;
    }

    std::optional< std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::try_get_variable_index(std::string name)
    {
        auto it = m_frame.blocks.back().variable_lookup_index.find(name);
        if (it == m_frame.blocks.back().variable_lookup_index.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    rpnx::general_coroutine< compiler, void > vm_procedure_from_canonical_functanoid_resolver::context_frame::run_value_destructor(std::size_t index)
    {
        auto type = get_variable_type(index);

        std::string variable_type_str = to_string(type);

        //std::cout << "Destroying value of type " << variable_type_str << std::endl;
        auto val = load_value_as_desctructable(index);
        subdotentity_reference destructor_symbol = {type, "DESTRUCTOR"};

        functanoid_reference destructor_reference;
        destructor_reference.callee = destructor_symbol;
        destructor_reference.parameters = {make_mref(type)};

        auto res = co_await m_resolver->gen_call(*this, destructor_symbol, {val});

        co_return;
    }
    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::run_value_constructor(std::size_t index, std::vector< vm_value > args)
    {
        auto type = get_variable_type(index);

        auto val = load_variable_as_new(index);
        subdotentity_reference constructor_symbol = {type, "CONSTRUCTOR"};
        args.insert(args.begin(), val);

        auto res = co_await m_resolver->gen_call(*this, constructor_symbol, args);

        co_return {};
    }

    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value_as_desctructable(std::size_t index)
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

        return load;
    }

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::set_return_value(vm_value val)
    {
       // std::cout << "Set return value: " << to_string(val) << std::endl;
        assert(m_frame.blocks.back().value_states[0].alive == false);

        co_await run_value_constructor(0, {std::move(val)});

        set_value_alive(0);

        co_return {};
    }
} // namespace rylang

rpnx::resolver_coroutine< rylang::compiler, rylang::vm_procedure > rylang::vm_procedure_from_canonical_functanoid_resolver::co_process(compiler* c, qualified_symbol_reference func_name)
{
    for (std::size_t i = 0; i < 10; i++)
    {
        std::cout << std::endl;
    }

    std::cout << "Begin processing" << std::endl;

    function_ast function_ast_v = co_await *c->lk_function_ast(func_name);

    vm_procedure vm_proc;
    vm_proc.mangled_name = mangle(func_name);
    vm_generation_frame_info frame;
    {
        context_frame ctx(this, func_name, c, frame, vm_proc);

        // If the function returns a value, that is the first variable

        if (function_ast_v.return_type)
        {
            vm_frame_variable var;
            var.name = "RETURN_VALUE";
            var.type = function_ast_v.return_type.value();
            var.storage.kind = storage_type::return_value;
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
            var.storage.kind = storage_type::argument;
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index[arg.name] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = true;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = true;
            vm_proc.interface.argument_types.push_back(arg.type);

            // TODO: Maybe consider adding ctx.add_argument instead of doing this inside this function
        }

        assert(frame.blocks.back().value_states[0].alive == false);

        // Then generate the body
        co_await build_generic(ctx, function_ast_v.body);

        co_await ctx.close();
    }
    auto ptr = this;
    assert(frame.blocks.empty());

    for (auto const& var : frame.variables)
    {
        assert(var.storage.valid());
        vm_proc.storage.push_back(var.storage);
    }

    co_return vm_proc;
}

rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_var_statement statement)
{

    std::string var_type_str = to_string(statement.type);

    auto canonical_type = co_await *ctx.compiler()->lk_canonical_type_from_contextual_type(statement.type, ctx.current_context());

    std::string canonical_var_str = to_string(canonical_type);

    std::vector< vm_value > args;
    for (auto& arg : statement.initializers)
    {
        auto val = co_await gen_value_generic(ctx, arg);
        args.push_back(val);
    }

    co_await ctx.construct_new_variable(statement.name, canonical_type, args);
    co_return;
}

rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_expression_statement statement)
{

    context_frame expr_ctx(ctx);

    // Each expression needs to have its own block to store temporary lvalues,
    // e.g. a := b + c, creates a temporary lvalue for the result of "b + c" before executing
    // the assignment itself.
    auto val = co_await gen_value_generic(expr_ctx, statement.expr);
    vm_execute_expression exc;
    exc.expr = val;
    expr_ctx.push(std::move(exc));
    co_await expr_ctx.close();
    co_return;
}
rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_generic(context_frame& ctx, expression expr)
{
    ctx.comment("context generate value generic");
    if (typeis< expression_symbol_reference >(expr))
    {
        co_return co_await gen_value(ctx, boost::get< expression_symbol_reference >(std::move(expr)));
    }
    else if (typeis< expression_binary >(expr))
    {
        co_return co_await gen_value(ctx, boost::get< expression_binary >(std::move(expr)));
    }
    else if (typeis< expression_call >(expr))
    {
        co_return co_await gen_value(ctx, boost::get< expression_call >(std::move(expr)));
    }
    else if (typeis< numeric_literal >(expr))
    {
        co_return co_await gen_value(ctx, boost::get< numeric_literal >(std::move(expr)));
    }
    else
    {
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }
    // gen_expression(ctx, expr);
    assert(false);
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_copy_assign expr)
{
    // TODO: Support overloading

    vm_value lhs = co_await gen_value_generic(ctx, expr.lhs);
    vm_value rhs = co_await gen_value_generic(ctx, expr.rhs);
    // TODO: support operator overloading

    // TODO: Support implicit casts

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

    co_return op;
}

rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_return_statement statement)
{

    if (statement.expr.has_value())
    {
        context_frame return_ctx(ctx);
        return_ctx.comment("return context!");
        auto retval = co_await gen_value_generic(return_ctx, statement.expr.value());

        auto vtype = vm_value_type(retval);
        // TODO: Support returning references?
        if (is_ref(vtype))
        {
            vtype = remove_ref(vtype);
            retval = vm_expr_dereference{retval, vtype};
        }
        co_await return_ctx.frame_return(std::move(retval));
        co_await return_ctx.close();

        co_return;
    }
    else
    {
        co_await ctx.frame_return();
        co_return;
    }
}

rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_if_statement statement)
{
    vm_if if_stmt;
    ctx.comment("entering build if statement");
    context_frame if_condition_ctx(ctx, if_stmt, context_frame::condition_tag);
    if_condition_ctx.comment("if condition context");
    auto condvalue = co_await gen_value_generic(if_condition_ctx, statement.condition);
    if_stmt.condition = std::move(condvalue);
    co_await if_condition_ctx.close();
    context_frame if_then_ctx(ctx, if_stmt, context_frame::then_tag);
    if_then_ctx.comment("if then context");
    co_await build_generic(if_then_ctx, statement.then_block);
    co_await if_then_ctx.close();

    if (statement.else_block.has_value())
    {
        context_frame if_else_ctx(ctx, if_stmt, context_frame::else_tag);
        if_else_ctx.comment("if else context");
        co_await build_generic(if_else_ctx, statement.else_block.value());

        if_else_ctx.comment("closing if else ctx");
        co_await if_else_ctx.close();
    }
    ctx.push(std::move(if_stmt));
    co_return;
}
rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_binary expr)
{

    auto lhs = co_await gen_value_generic(ctx, expr.lhs);
    auto rhs = co_await gen_value_generic(ctx, expr.rhs);

    qualified_symbol_reference lhs_type = boost::apply_visitor(vm_value_type_vistor(), lhs);
    qualified_symbol_reference rhs_type = boost::apply_visitor(vm_value_type_vistor(), rhs);

    qualified_symbol_reference lhs_underlying_type = remove_ref(lhs_type);
    qualified_symbol_reference rhs_underlying_type = remove_ref(rhs_type);

    qualified_symbol_reference lhs_function = subdotentity_reference{lhs_underlying_type, "OPERATOR" + expr.operator_str};
    qualified_symbol_reference rhs_function = subdotentity_reference{rhs_underlying_type, "OPERATOR" + expr.operator_str + "RHS"};

    call_parameter_information lhs_param_info{{lhs_type, rhs_type}};
    call_parameter_information rhs_param_info{{rhs_type, lhs_type}};
    auto lhs_exists_and_callable_with = co_await *ctx.compiler()->lk_functum_exists_and_is_callable_with(lhs_function, lhs_param_info);

    if (lhs_exists_and_callable_with)
    {
        co_return co_await gen_call(ctx, lhs_function, std::vector< vm_value >{lhs, rhs});
    }

    auto rhs_exists_and_callable_with = co_await *ctx.compiler()->lk_functum_exists_and_is_callable_with(rhs_function, rhs_param_info);

    if (rhs_exists_and_callable_with)
    {
        co_return co_await gen_call(ctx, rhs_function, std::vector< vm_value >{rhs, lhs});
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_call expr)
{
    vm_value callee = co_await gen_value_generic(ctx, expr.callee);
    std::vector< vm_value > args;

    for (auto& arg_ast : expr.args)
    {
        // TODO: Translate arg types from references
        vm_value arg_val = co_await gen_value_generic(ctx, arg_ast);
        args.push_back(arg_val);
    }

    auto cv = co_await gen_call_expr(ctx, callee, args);
    co_return cv;
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::expression_symbol_reference expr)
{
    bool is_possibly_frame_value = typeis< subentity_reference >(expr.symbol) && typeis< context_reference >(boost::get< subentity_reference >(expr.symbol).parent);

    // Frame values are sub-entities of the current context.
    if (is_possibly_frame_value)
    {
        std::string name = boost::get< subentity_reference >(expr.symbol).subentity_name;

        std::optional< vm_value > val = ctx.try_load_variable(name);

        if (val.has_value())
        {
            co_return val.value();
        }
    }

    // assert(ctx.current_context() == m_func_name);
    auto canonical_symbol = co_await *ctx.compiler()->lk_canonical_type_from_contextual_type(expr.symbol, ctx.funcname());

    std::string symbol_str = to_string(canonical_symbol);
    // TODO: Check if global variable
    bool is_global_variable = false;
    bool is_function = true; // This might not actually be true

    vm_expr_bound_value result;
    result.function_ref = canonical_symbol;
    result.value = void_value{};

    co_return result;
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_implicit_conversion(context_frame& ctx, rylang::vm_value from, rylang::qualified_symbol_reference to)
{
    auto from_type = vm_value_type(from);

    if (from_type == to)
    {
        co_return from;
    }

    if (remove_ref(from_type) == to)
    {
        co_return co_await gen_ref_to_value(ctx, from);
    }

    else if (from_type == remove_ref(to))
    {
        co_return co_await gen_value_to_ref(ctx, from, to);
    }

    if (is_ref(from_type) && is_ref(to) && remove_ref(from_type) == remove_ref(to))
    {
        co_return vm_expr_reinterpret{from, to};
    }

    auto underlying_to_type = remove_ref(to);
    if (typeis< primitive_type_integer_reference >(to) && typeis< vm_expr_literal >(from))
    {
        vm_value result = gen_conversion_to_integer(ctx, boost::get< vm_expr_literal >(from), boost::get< primitive_type_integer_reference >(to));

        if (is_ref(to))
        {
            co_return co_await gen_value_to_ref(ctx, result, to);
        }
        else
        {
            co_return result;
        }
    }

    // TODO: Allowed integer conversions, etc

    throw std::runtime_error("Cannot convert between these types");
}
rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value_to_ref(context_frame& ctx, rylang::vm_value from, rylang::qualified_symbol_reference to_type)
{
    assert(is_canonical(to_type));
    std::size_t index = co_await ctx.adopt_value_as_temporary(from);
    co_return ctx.load_temporary(index);
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_ref_to_value(context_frame& ctx, rylang::vm_value val)
{
    auto arg_type = vm_value_type(val);
    qualified_symbol_reference to_type = remove_ref(arg_type);
    assert(is_canonical(arg_type));
    // TODO: Copy constructor here
    co_return vm_expr_dereference{val, to_type};
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_expr(context_frame& ctx, vm_value callee, std::vector< vm_value > values)
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

    co_return co_await gen_call(ctx, callee_func, values);
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call(context_frame& ctx, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{
    call_parameter_information call_set;
    call_set.argument_types = {};

    for (vm_value const& val : call_args)
    {
        call_set.argument_types.push_back(vm_value_type(val));
    }
    // TODO: Check if function parameter set already specified.

    call_parameter_information overload = co_await *ctx.compiler()->lk_function_overload_selection(callee, call_set);

    functanoid_reference overload_selected_ref;
    overload_selected_ref.callee = callee;
    overload_selected_ref.parameters = overload.argument_types;

    co_return co_await gen_call_functanoid(ctx, overload_selected_ref, call_args);
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_default_constructor(context_frame& ctx, rylang::qualified_symbol_reference type, std::vector< vm_value > values)
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

        co_return void_value{};
    }

    class_layout layout = co_await *ctx.compiler()->lk_class_layout_from_canonical_chain(type);

    auto this_obj = values.at(0);

    for (auto const& field : layout.fields)
    {
        vm_expr_access_field access;
        access.type = make_mref(field.type);
        access.base = this_obj;
        access.offset = field.offset;
        auto field_constructor = subdotentity_reference{field.type, "CONSTRUCTOR"};
        co_await gen_call(ctx, field_constructor, {access});
    }

    co_return void_value();
}

rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_while_statement statement)
{
    vm_while whl_statement;
    context_frame while_condition_frame(ctx, whl_statement, context_frame::condition_tag);
    whl_statement.condition = co_await gen_value_generic(while_condition_frame, statement.condition);

    context_frame while_loop_ctx(ctx, whl_statement, context_frame::loop_tag);
    for (auto& i : statement.loop_block.statements)
    {
        co_await build_generic(while_loop_ctx, i);
    }
    co_await while_loop_ctx.close();
    co_return;
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, rylang::numeric_literal expr)
{
    co_return vm_expr_literal{expr.value};
}
rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_call_functanoid(context_frame& ctx, rylang::qualified_symbol_reference callee, std::vector< vm_value > call_args)
{

    functanoid_reference const& overload_selected_ref = boost::get< functanoid_reference >(callee);
    std::string overload_string = to_string(callee) + "  " + to_string(overload_selected_ref);

    auto args = co_await gen_preinvoke_conversions(ctx, std::move(call_args), overload_selected_ref.parameters);
    std::optional< vm_value > value_maybe = co_await try_gen_call_functanoid_builtin(ctx, callee, args);

    if (value_maybe.has_value())
    {
        co_return value_maybe.value();
    }

    co_return co_await gen_invoke(ctx, overload_selected_ref, std::move(args));
}

rpnx::general_coroutine< rylang::compiler, rylang::vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_invoke(context_frame& ctx, functanoid_reference const& overload_selected_ref, std::vector< vm_value > call_args)
{

    vm_expr_call call;

    std::string typestr = to_string(overload_selected_ref);
    //std::cout << "gen invoke of " << typestr << std::endl;

    call.mangled_procedure_name = mangle(overload_selected_ref);
    call.functanoid = overload_selected_ref;

    call.interface = vm_procedure_interface{};
    call.interface.argument_types = overload_selected_ref.parameters;

    auto return_type = co_await *ctx.compiler()->lk_functanoid_return_type(overload_selected_ref);

    if (!typeis< void_type >(return_type))
    {
        call.interface.return_type = return_type;
    }

    call.arguments = std::move(call_args);

    assert(call.mangled_procedure_name != "");

    ctx.procedure().invoked_functanoids.insert(overload_selected_ref);
    if (call.interface.return_type.has_value())
    {
        auto temp_index = co_await ctx.create_temporary_storage(call.interface.return_type.value());

        auto tempval = ctx.load_temporary_as_new(temp_index);
        vm_store call_result;
        call_result.type = call.interface.return_type.value();
        call_result.where = tempval;
        call_result.what = std::move(call);
        ctx.push(call_result);
        ctx.set_value_alive(temp_index);
        co_return ctx.load_temporary(temp_index);
    }
    else
    {
        ctx.push(vm_execute_expression{call});
        co_return void_value();
    }
}

rpnx::general_coroutine< rylang::compiler, std::vector< vm_value > > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_preinvoke_conversions(context_frame& ctx, std::vector< vm_value > values, std::vector< qualified_symbol_reference > const& to_types)
{
    // TODO: Add support for default parameters.
    std::vector< vm_value > result;
    assert(values.size() == to_types.size());

    for (std::size_t i = 0; i < values.size(); i++)
    {
        auto converted_value = co_await gen_implicit_conversion(ctx, values.at(i), to_types.at(i));
        result.push_back(converted_value);
    }
    co_return result;
}
rpnx::general_coroutine< rylang::compiler, std::optional< rylang::vm_value > > rylang::vm_procedure_from_canonical_functanoid_resolver::try_gen_call_functanoid_builtin(context_frame& ctx, rylang::qualified_symbol_reference callee_set, std::vector< vm_value > values)
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
            co_return op;
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
                co_return void_value{};
            }

            else if (values.size() == 2)
            {
                // copy constructor
                vm_expr_store initalizer;
                vm_value arg_to_copy = values.at(1);
                qualified_symbol_reference arg_copy_type = vm_value_type(arg_to_copy);
                // TODO: conversion to integer?
                if (arg_copy_type != remove_ref(arg_type))
                {
                    throw std::runtime_error("Unimplemented integer of different type passed to int constructor");
                }

                initalizer.what = arg_to_copy;
                initalizer.where = arg;
                initalizer.type = int_type;
                ctx.push(vm_execute_expression{initalizer});
                co_return void_value{};
            }
        }
        else if (subdot.subdotentity_name == "CONSTRUCTOR")
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen = co_await *ctx.compiler()->lk_class_should_autogen_default_constructor(parent_type);

            if (!should_autogen)
            {
                co_return std::nullopt;
            }

            co_return co_await gen_default_constructor(ctx, parent_type, values);
        }
        else if (subdot.subdotentity_name == "DESTRUCTOR")
        {
            // TODO: Allow this to be provided by users.

            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            // auto should_autogen_dp = get_dependency(
            //    [&]
            //    {
            //        return ctx.compiler()->lk_class_should_autogen_default_constructor(parent_type);
            //    });

            bool should_autogen = true; // should_autogen_dp->get();
            if (!should_autogen)
            {
                co_return std::nullopt;
            }

            co_return co_await gen_default_destructor(ctx, parent_type, values);
        }
    }

    co_return std::nullopt;
}

rylang::vm_value rylang::vm_procedure_from_canonical_functanoid_resolver::gen_conversion_to_integer(context_frame& ctx, rylang::vm_expr_literal val, rylang::primitive_type_integer_reference to_type)
{
    vm_expr_load_literal result = vm_expr_load_literal{val.literal, to_type};

    return result;
}

rpnx::general_coroutine< compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, rylang::function_block statement)
{
    context_frame block_frame(ctx);
    for (auto& i : statement.statements)
    {
        co_await build_generic(block_frame, i);

        // TODO: Maybe consider not doing this later for adding GOTO statements.
        if (typeis< function_return_statement >(i))
        {
            break;
        }
    }
    co_await block_frame.close();
    co_return;
}
rpnx::general_coroutine< compiler, vm_value > rylang::vm_procedure_from_canonical_functanoid_resolver::gen_default_destructor(context_frame& ctx, rylang::qualified_symbol_reference type, std::vector< vm_value > values)
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
        vm_expr_store set_poison;
        set_poison.type = type;
        set_poison.where = values.at(0);
        set_poison.what = vm_expr_poison{type};
        ctx.push(vm_execute_expression{set_poison});

        co_return void_value{};
    }

    if (is_primitive(type))
    {
        vm_expr_store set_poison;
        set_poison.type = type;
        set_poison.where = values.at(0);
        set_poison.what = vm_expr_poison{type};
        ctx.push(vm_execute_expression{set_poison});

        co_return void_value{};
    }

    class_layout layout = co_await *ctx.compiler()->lk_class_layout_from_canonical_chain(type);

    vm_value this_obj = values.at(0);

    for (auto const& field : layout.fields)
    {
        vm_expr_access_field access;
        access.type = make_mref(field.type);
        access.base = this_obj;
        access.offset = field.offset;

        auto field_destructor = subdotentity_reference{field.type, "DESTRUCTOR"};

        co_await gen_call(ctx, field_destructor, {access});
    }

    co_return void_value();
}
rpnx::general_coroutine< rylang::compiler, void > rylang::vm_procedure_from_canonical_functanoid_resolver::build_generic(rylang::vm_procedure_from_canonical_functanoid_resolver::context_frame& ctx, rylang::function_statement statement)
{

    if (typeis< function_var_statement >(statement))
    {
        function_var_statement var_stmt = boost::get< function_var_statement >(statement);
        co_await build(ctx, var_stmt);
        co_return;
    }
    else if (typeis< function_expression_statement >(statement))
    {
        function_expression_statement expr_stmt = boost::get< function_expression_statement >(statement);
        co_await build(ctx, expr_stmt);
        co_return;
    }
    else if (typeis< function_if_statement >(statement))
    {
        function_if_statement if_stmt = boost::get< function_if_statement >(statement);
        co_await build(ctx, if_stmt);
        co_return;
    }
    else if (typeis< function_while_statement >(statement))
    {
        function_while_statement while_stmt = boost::get< function_while_statement >(statement);
        co_await build(ctx, while_stmt);
        co_return;
    }
    else if (typeis< function_return_statement >(statement))
    {
        function_return_statement return_stmt = boost::get< function_return_statement >(statement);
        co_await build(ctx, return_stmt);
        co_return;
    }
    else if (typeis< function_block >(statement))
    {
        function_block block_stmt = boost::get< function_block >(statement);
        context_frame block_ctx(ctx);
        co_await build(ctx, block_stmt);
        co_await block_ctx.close();
        co_return;
    }
    else
    {
        throw std::runtime_error("Unknown function statement type");
    }
    throw std::runtime_error("unimplemented");
}
