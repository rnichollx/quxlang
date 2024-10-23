// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/res/vm_procedure_from_canonical_functanoid_resolver.hpp"

#include "quxlang/data/vm_generation_frameinfo.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/manipulators/mangler.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/manipulators/vmmanip.hpp"
#include "quxlang/operators.hpp"
#include "quxlang/variant_utils.hpp"

#include "quxlang/data/type_placement_info.hpp"

#include <exception>

// TODO: Debugging, remove this
#include "quxlang/to_pretty_string.hpp"
#include "rpnx/debug.hpp"

#include <iostream>

#include <quxlang/compiler.hpp>

using namespace quxlang;

namespace quxlang
{

    vm_procedure_from_canonical_functanoid_resolver::context_frame::context_frame(vm_procedure_from_canonical_functanoid_resolver* res, type_symbol func, class compiler* c, vm_generation_frame_info& frame, vm_procedure& proc)
        : m_c(c)

        , m_frame(frame)
        , m_ctx(func)
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
        vm_expr_load_reference load;
        load.index = index;
        auto vartype = m_frame.variables.at(index).type;

        std::string vartype_str = to_string(vartype);

        assert(is_canonical(vartype));

        if (is_ref(vartype))
        {
            load.type = instance_pointer_type{vartype};

            auto deref = vm_expr_dereference{load, vartype};
            return deref;
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

    type_symbol vm_procedure_from_canonical_functanoid_resolver::context_frame::current_context() const
    {
        return m_frame.context;
    }

    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_value_storage(std::optional< std::string > name, type_symbol type)
    {
        bool temp = !(name.has_value());

        std::string var_type_str = to_string(type);

        type_placement_info type_placement_info_v = co_await *get_compiler()->lk_type_placement_info_from_canonical_type(type);

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
        std::string typestr = to_string(type);
        var.type = type;
        var.is_temporary = temp;
        var.storage = storage;
        assert(var.storage.valid());
        vm_expr_load_reference load;
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

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::construct_new_variable(std::string name, type_symbol type, vm_callargs args)
    {

        auto index = co_await create_variable_storage(name, type);

        // auto val = load_variable_as_new(index);

        co_await run_value_constructor(index, args);

        set_value_alive(index);

        co_return {};
    }

    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value(std::size_t index, bool alive, bool temp)
    {
        auto th = this;
        vm_expr_load_reference load;
        load.index = index;
        auto vartype = m_frame.variables.at(index).type;

        assert(m_frame.variables.at(index).is_temporary == temp);
        assert(m_frame.blocks.back().value_states.at(index).alive == alive);

        std::string vartype_str = to_string(vartype);

        assert(is_canonical(vartype));

        if (is_ref(vartype))
        {
            load.type = instance_pointer_type{vartype};
            auto deref = vm_expr_dereference{load, vartype};

            return deref;
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
        // std::cout << "set < " << index << " > alive" << std::endl;
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

    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_variable_storage(std::string name, type_symbol type)
    {
        return create_value_storage(name, type);
    }

    rpnx::general_coroutine< compiler, std::size_t > vm_procedure_from_canonical_functanoid_resolver::context_frame::create_temporary_storage(type_symbol type)
    {
        return create_value_storage(std::nullopt, type);
    }

    type_symbol vm_procedure_from_canonical_functanoid_resolver::context_frame::get_variable_type(std::size_t index)
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

        // std::cout << "Destroying value of type " << variable_type_str << std::endl;
        auto val = load_value_as_desctructable(index);
        submember destructor_symbol = {type, "DESTRUCTOR"};

        instantiation_type destructor_reference;
        destructor_reference.callee = destructor_symbol;
        destructor_reference.parameters = {.named = {{"THIS", make_mref(type)}}};

        auto res = co_await m_resolver->gen_call(*this, destructor_symbol, vm_callargs{.named = {{"THIS", val}}});

        co_return;
    }

    rpnx::general_coroutine< compiler, std::monostate > vm_procedure_from_canonical_functanoid_resolver::context_frame::run_value_constructor(std::size_t index, vm_callargs args)
    {
        auto type = get_variable_type(index);

        auto val = load_variable_as_new(index);
        submember constructor_symbol = {type, "CONSTRUCTOR"};
        args.named["THIS"] = val;

        co_await m_resolver->gen_call(*this, constructor_symbol, args);

        co_return {};
    }

    vm_value vm_procedure_from_canonical_functanoid_resolver::context_frame::load_value_as_desctructable(std::size_t index)
    {
        bool alive = true;
        vm_expr_load_reference load;
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



        co_await run_value_constructor(0, {.positional = {std::move(val)}});
        set_value_alive(0);

        rpnx::unimplemented();
        co_return {};
    }
} // namespace quxlang

rpnx::resolver_coroutine< quxlang::compiler, quxlang::vm_procedure > quxlang::vm_procedure_from_canonical_functanoid_resolver::co_process(compiler* c, instantiation_type func_name)
{
    for (std::size_t i = 0; i < 3; i++)
    {
        std::cout << std::endl;
    }

    QUXLANG_DEBUG({ std::cout << "Begin processing" << std::endl; });

    QUX_CO_GETDEP(insta, functum_instanciation, (func_name));

    if (!insta.has_value())
    {
        throw std::logic_error("Could not resolve function instanciation");
    }

    QUX_CO_GETDEP(sel, functum_select_function, (func_name));

    if (!sel.has_value())
    {
        throw std::logic_error("something wrong this should not be possible");
    }

    std::string dbg_func_name = quxlang::to_string(func_name);

    ast2_function_declaration function_ast_v = (co_await QUX_CO_DEP(function_declaration, (sel.value()))).value();

    type_symbol functum_reference = sel->templexoid;

    std::string dbg_functum_reference_name = quxlang::to_string(functum_reference);

    bool is_member = false;

    std::optional< type_symbol > parent_type;

    std::optional< std::string > func_name_str;

    if (typeis< submember >(functum_reference))
    {
        is_member = true;
        parent_type = as< submember >(functum_reference).of;
        func_name_str = as< submember >(functum_reference).name;
    }
    else
    {
        assert(typeis< subsymbol >(functum_reference));
        parent_type = as< subsymbol >(functum_reference).of;
        func_name_str = as< subsymbol >(functum_reference).name;
    }
    // TODO: Support more member function logic, e.g. templates

    std::optional< vm_value > this_value;
    std::optional< type_symbol > this_type;
    std::optional< type_symbol > thistype_type;
    vm_procedure vm_proc;
    vm_proc.mangled_name = mangle(func_name);
    vm_generation_frame_info frame;
    {
        context_frame ctx(this, func_name, c, frame, vm_proc);

        // If the function returns a value, that is the first variable

        if (function_ast_v.definition.return_type)
        {
            vm_frame_variable var;
            var.name = "RESULT";
            // TODO: check any "result_val" references
            var.type = function_ast_v.definition.return_type.value();
            var.storage.kind = storage_type::return_value;
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index["return"] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = false;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = false;
            // TODO: use a lookup for the return type instead
            vm_proc.interface.return_type = function_ast_v.definition.return_type.value();
        }

        std::vector< std::string > param_names = co_await QUX_CO_DEP(function_positional_parameter_names, (sel.value()));

        vm_proc.interface.argument_types = insta->parameters;

        for (auto const& param : insta->parameters.named)
        {
            std::string name = param.first;

            auto arg_type = param.second;

            // TODO: consider pass by pointer of large values instead of by value?
            vm_frame_variable var;
            // NOTE: Make sure that we can handle contextual types in arg list.
            //  They should be decontextualized somewhere else probably.. so maybe this will
            //  be impossible.

            // TODO: Make sure no name conflicts are allowed.
            assert(!qualified_is_contextual(arg_type));

            std::string arg_name = name;
            var.type = arg_type;
            // TODO: handle references correctly
            var.get_addr = vm_expr_load_reference{frame.variables.size(), make_mref(var.type)};
            var.storage.kind = storage_type::argument;
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index[arg_name] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = true;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = true;
        }

        for (std::size_t i = 0; i < insta->parameters.positional.size(); i++)
        {
            auto arg_type = insta->parameters.positional.at(i);

            // TODO: Check that arg_type matches arg.type

            // TODO: consider pass by pointer of large values instead of by value?
            vm_frame_variable var;
            // NOTE: Make sure that we can handle contextual types in arg list.
            //  They should be decontextualized somewhere else probably.. so maybe this will
            //  be impossible.

            // TODO: Make sure no name conflicts are allowed.
            assert(!qualified_is_contextual(arg_type));

            std::string arg_name = param_names.at(i);
            // TODO: Arg.name
            // var.name =
            var.type = arg_type;
            var.get_addr = vm_expr_load_reference{frame.variables.size(), instance_pointer_type{.target = var.type}};
            var.storage.kind = storage_type::argument;
            frame.variables.push_back(var);
            assert(!frame.blocks.empty());
            frame.blocks.back().variable_lookup_index[arg_name] = frame.variables.size() - 1;
            frame.blocks.back().value_states[frame.variables.size() - 1].alive = true;
            frame.blocks.back().value_states[frame.variables.size() - 1].this_frame = true;

            // TODO: Maybe consider adding ctx.add_argument instead of doing this inside this function
        }

        assert(!function_ast_v.definition.return_type.has_value() || frame.blocks.back().value_states[0].alive == false);

        bool is_constructor = func_name_str.has_value() && func_name_str.value() == "CONSTRUCTOR" && is_member;
        bool is_destructor = false;

        if (is_constructor)
        {
            // TODO: Refector this into separate function?
            // assert(thistype_type.has_value());
            class_layout this_layout = co_await QUX_CO_DEP(class_layout, (*thistype_type));
            std::set< std::string > intialized_members;
            for (ast2_function_delegate& delegate : function_ast_v.definition.delegates)
            {
                // TODO: Support intializing base classes (after we add inheritance)

                auto& target = delegate.target;

                if (typeis< submember >(target) && (as< submember >(target).of == type_symbol(context_reference{})))
                {
                    std::string name = as< submember >(target).name;

                    // We need to loop over the layout fields and find this member
                    for (class_field_info& field : this_layout.fields)
                    {
                        if (field.name == name)
                        {
                            // We should generate the args for this call and then call the constructor

                            vm_callargs args;

                            // The first arg is the

                            auto th = gen_this(ctx);

                            auto get_element_ptr = vm_expr_access_field{th, field.offset, make_mref(field.type)};

                            args.positional.push_back(get_element_ptr);

                            for (auto& arg_expr : delegate.args)
                            {
                                auto val = co_await gen_value_generic(ctx, arg_expr);
                                args.positional.push_back(val);
                            }

                            co_await gen_call(ctx, submember{field.type, "CONSTRUCTOR"}, args);

                            intialized_members.insert(name);
                        }
                    }
                }
            }

            for (class_field_info& field : this_layout.fields)
            {
                if (intialized_members.contains(field.name))
                {
                    continue;
                }
                vm_callargs args;

                auto th = gen_this(ctx);

                auto get_element_ptr = vm_expr_access_field{th, field.offset, make_mref(field.type)};

                args.positional.push_back(get_element_ptr);

                co_await gen_call(ctx, submember{field.type, "CONSTRUCTOR"}, args);
            }

            // gen_default_constructor(ctx, parent_type.value(), {this_value.value()});
        }

        // Then generate the body
        co_await build_generic(ctx, function_ast_v.definition.body);

        if (is_destructor) {}

        // Implied return on void functions
        if (!function_ast_v.definition.return_type.has_value())
        {
            co_await ctx.frame_return();
        }
        // TODO: Currently undefined behavior, (does not compile with llvm)
        //  instead, insert termination here.

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

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_var_statement statement)
{

    std::string var_type_str = to_string(statement.type);

    auto canonical_type = co_await *ctx.get_compiler()->lk_canonical_symbol_from_contextual_symbol(statement.type, ctx.current_context());

    std::string canonical_var_str = to_string(canonical_type);

    vm_callargs args;
    for (auto& arg : statement.initializers)
    {
        auto val = co_await gen_value_generic(ctx, arg);
        args.positional.push_back(val);
    }

    co_await ctx.construct_new_variable(statement.name, canonical_type, args);
    co_return;
}

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_expression_statement statement)
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

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value_generic(context_frame& ctx, expression expr)
{
    ctx.comment("context generate value generic");
    if (typeis< expression_symbol_reference >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_symbol_reference >(std::move(expr)));
    }
    else if (typeis< expression_binary >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_binary >(std::move(expr)));
    }
    else if (typeis< expression_call >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_call >(std::move(expr)));
    }
    else if (typeis< expression_numeric_literal >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_numeric_literal >(std::move(expr)));
    }
    else if (typeis< expression_thisdot_reference >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_thisdot_reference >(std::move(expr)));
    }
    else if (typeis< expression_this_reference >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_this_reference >(std::move(expr)));
    }
    else if (typeis< expression_dotreference >(expr))
    {
        co_return co_await gen_value(ctx, as< expression_dotreference >(std::move(expr)));
    }

    else
    {
        throw std::logic_error("Unimplemented handler for " + std::string(expr.type().name()));
    }
    // gen_expression(ctx, expr);
    assert(false);
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, quxlang::expression_copy_assign expr)
{
    // TODO: Support overloading

    vm_value lhs = co_await gen_value_generic(ctx, expr.lhs);
    vm_value rhs = co_await gen_value_generic(ctx, expr.rhs);
    // TODO: support operator overloading

    // TODO: Support implicit casts

    type_symbol lhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), lhs);
    std::string lhs_type_string = to_string(lhs_type);

    type_symbol rhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), rhs);
    std::string rhs_type_string = to_string(rhs_type);

    if (!is_ref(lhs_type))
    {
        throw std::logic_error("Cannot assign to non-reference");
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

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_return_statement statement)
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

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_if_statement statement)
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

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, quxlang::expression_binary expr)
{

    auto lhs = co_await gen_value_generic(ctx, expr.lhs);
    auto rhs = co_await gen_value_generic(ctx, expr.rhs);

    type_symbol lhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), lhs);
    type_symbol rhs_type = rpnx::apply_visitor< type_symbol >(vm_value_type_vistor(), rhs);

    type_symbol lhs_underlying_type = remove_ref(lhs_type);
    type_symbol rhs_underlying_type = remove_ref(rhs_type);

    type_symbol lhs_function = submember{lhs_underlying_type, "OPERATOR" + expr.operator_str};
    type_symbol rhs_function = submember{rhs_underlying_type, "OPERATOR" + expr.operator_str + "RHS"};

    calltype lhs_param_info{.named = {{"THIS", lhs_type}}, .positional = {rhs_type}};
    calltype rhs_param_info{.named = {{"THIS", rhs_type}}, .positional = {lhs_type}};
    auto lhs_exists_and_callable_with = co_await *ctx.get_compiler()->lk_functum_exists_and_is_callable_with({.callee = lhs_function, .parameters = lhs_param_info});

    if (lhs_exists_and_callable_with)
    {
        co_return co_await gen_call(ctx, lhs_function, vm_callargs{.named = {{"THIS", lhs}}, .positional = {rhs}});
    }

    auto rhs_exists_and_callable_with = co_await *ctx.get_compiler()->lk_functum_exists_and_is_callable_with({.callee = rhs_function, .parameters = rhs_param_info});

    if (rhs_exists_and_callable_with)
    {
        co_return co_await gen_call(ctx, rhs_function, vm_callargs{.named = {{"THIS", rhs}}, .positional = {lhs}});
    }

    throw std::logic_error("Found neither " + to_string(lhs_function) + " callable with (" + to_string(lhs_type) + ", " + to_string(rhs_type) + ") nor " + to_string(rhs_function) + " callable with (" + to_string(rhs_type) + ", " + to_string(lhs_type) + ")");
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, quxlang::expression_call expr)
{
    vm_value callee = co_await gen_value_generic(ctx, expr.callee);
    vm_callargs args;

    for (auto& arg_ast : expr.args)
    {
        // TODO: Translate arg types from references
        rpnx::unimplemented();
        //vm_value arg_val = co_await gen_value_generic(ctx, arg_ast);
      //  args.positional.push_back(arg_val);
    }
    // TODO: Support named arguments

    auto cv = co_await gen_call_expr(ctx, callee, args);
    co_return cv;
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, quxlang::expression_symbol_reference expr)
{
    bool is_possibly_frame_value = typeis< subsymbol >(expr.symbol) && typeis< context_reference >(as< subsymbol >(expr.symbol).of);

    // Frame values are sub-entities of the current context.
    if (is_possibly_frame_value)
    {
        std::string name = as< subsymbol >(expr.symbol).name;

        std::optional< vm_value > val = ctx.try_load_variable(name);

        if (val.has_value())
        {
            co_return val.value();
        }
    }

    // assert(ctx.current_context() == m_func_name);
    auto canonical_symbol = co_await *ctx.get_compiler()->lk_canonical_symbol_from_contextual_symbol(expr.symbol, ctx.funcname());

    std::string symbol_str = to_string(canonical_symbol);
    // TODO: Check if global variable
    bool is_global_variable = false;
    bool is_function = true; // This might not actually be true

    vm_expr_bound_value result;
    result.function_ref = canonical_symbol;
    result.value = void_value{};

    co_return result;
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_implicit_conversion(context_frame& ctx, quxlang::vm_value from, quxlang::type_symbol to)
{
    auto from_type = vm_value_type(from);

    QUXLANG_DEBUG(std::string from_type_str = to_string(from_type));
    QUXLANG_DEBUG(std::string to_type_str = to_string(to));

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
    if (typeis< int_type >(to) && typeis< vm_expr_literal >(from))
    {
        vm_value result = gen_conversion_to_integer(ctx, as< vm_expr_literal >(from), as< int_type >(to));

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

    throw std::logic_error("Cannot convert between these types");
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value_to_ref(context_frame& ctx, quxlang::vm_value from, quxlang::type_symbol to_type)
{
    assert(is_canonical(to_type));
    std::size_t index = co_await ctx.adopt_value_as_temporary(from);
    co_return ctx.load_temporary(index);
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_ref_to_value(context_frame& ctx, quxlang::vm_value val)
{
    auto arg_type = vm_value_type(val);
    type_symbol to_type = remove_ref(arg_type);
    assert(is_canonical(arg_type));
    // TODO: Copy constructor here
    co_return vm_expr_dereference{val, to_type};
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_call_expr(context_frame& ctx, vm_value callee, vm_callargs values)
{
    // TODO: support overloaded operator() of non-functions
    if (!typeis< vm_expr_bound_value >(callee))
    {
        throw std::logic_error("Cannot call non-function reference");
    }

    vm_expr_bound_value callee_binding_value = as< vm_expr_bound_value >(callee);

    vm_value callee_value = callee_binding_value.value;
    type_symbol callee_func = callee_binding_value.function_ref;

    assert(is_canonical(callee_func));

    // TODO: Consider omitting the callee if size of type is 0
    if (!typeis< void_value >(callee_value))
    {
        values.named.insert({"THIS", callee_value});
    }

    co_return co_await gen_call(ctx, callee_func, std::move(values));
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_call(context_frame& ctx, quxlang::type_symbol callee, vm_callargs call_args)
{
    calltype call_set;

    for (vm_value const& val : call_args.positional)
    {
        call_set.positional.push_back(vm_value_type(val));
    }
    for (auto const& [name, val] : call_args.named)
    {
        call_set.named[name] = vm_value_type(val);
    }
    // TODO: Check if function parameter set already specified.

    // TODO: Reimplement this

    // assert(false);
    auto selected_overload = co_await *ctx.get_compiler()->lk_functum_instanciation(instantiation_type{.callee = callee, .parameters = call_set});

    if (!selected_overload.has_value())
    {
        throw std::logic_error("No overload found for " + to_string(callee));
    }

    // instanciation_reference overload_selected_ref;
    // overload_selected_ref.callee = callee;
    // overload_selected_ref.parameters = overload.argument_types;

    co_return co_await gen_call_functanoid(ctx, selected_overload.value(), call_args);
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_default_constructor(context_frame& ctx, quxlang::type_symbol type, vm_callargs values)
{
    // TODO: make default constructing references an error

    assert(!is_ref(type));

    std::string val;

    for (auto& i : values.positional)
    {
        val += to_string(i) + " ";
    }
    // TODO: Why are we using vm_callargs for default constructor???

    // TODO: Rewrite this function

    // if (values.size() != 1)
    // {
    //    throw std::logic_error("Invalid number of arguments to default constructor");
    // }

    assert(values.positional.empty());
    assert(values.named.size() == 1);

    auto const& thisvalue = values.named.at("THIS");
    auto arg_type = vm_value_type(thisvalue);
    assert(arg_type == make_mref(type) || arg_type == make_wref(type));

    if (is_ptr(type))
    {
        vm_expr_store set_nullptr;
        set_nullptr.type = type;
        set_nullptr.where = thisvalue;
        set_nullptr.what = vm_expr_load_literal{"NULLPTR", type};
        ctx.push(vm_execute_expression{set_nullptr});

        co_return void_value{};
    }

    class_layout layout = co_await *ctx.get_compiler()->lk_class_layout(type);

    for (auto const& field : layout.fields)
    {
        vm_expr_access_field access;
        access.type = make_mref(field.type);
        access.base = thisvalue;
        access.offset = field.offset;
        auto field_constructor = submember{field.type, "CONSTRUCTOR"};
        co_await gen_call(ctx, field_constructor, {.named = {{"THIS", access}}});
    }

    co_return void_value();
}

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_while_statement statement)
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

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, quxlang::expression_numeric_literal expr)
{
    co_return vm_expr_literal{expr.value};
}

rpnx::general_coroutine< compiler, vm_value > vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, expression_this_reference expr)
{
    co_return gen_this(ctx);
}

rpnx::general_coroutine< compiler, vm_value > vm_procedure_from_canonical_functanoid_resolver::gen_value(context_frame& ctx, expression_thisdot_reference expr)
{
    auto thisval = gen_this(ctx);
    return gen_access_field(ctx, thisval, expr.field_name);
}

vm_value vm_procedure_from_canonical_functanoid_resolver::gen_this(context_frame& ctx)
{
    return ctx.load_variable("THIS");
}

rpnx::general_coroutine< compiler, vm_value > vm_procedure_from_canonical_functanoid_resolver::gen_access_field(context_frame& ctx, vm_value val, std::string field_name)
{

    auto thisreftype = vm_value_type(val);
    assert(is_ref(thisreftype));
    auto thistype = remove_ref(thisreftype);

    class_layout layout = co_await *ctx.get_compiler()->lk_class_layout(thistype);

    for (class_field_info const& field : layout.fields)
    {
        if (field.name == field_name)
        {
            vm_expr_access_field access;
            if (typeis< mvalue_reference >(thisreftype))
            {
                access.type = make_mref(field.type);
            }
            else if (typeis< tvalue_reference >(thisreftype))
            {
                access.type = make_tref(field.type);
            }
            else if (typeis< wvalue_reference >(thisreftype))
            {
                access.type = make_wref(field.type);
            }
            else if (typeis< cvalue_reference >(thisreftype))
            {
                access.type = make_cref(field.type);
            }
            else
            {
                assert(false);
            }
            access.base = val;
            access.offset = field.offset;
            co_return access;
        }
    }

    throw std::logic_error("No such field");
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_call_functanoid(context_frame& ctx, quxlang::type_symbol callee, quxlang::vm_callargs call_args)
{

    instantiation_type const& overload_selected_ref = as< instantiation_type >(callee);
    std::string overload_string = to_string(callee);

    auto args = co_await gen_preinvoke_conversions(ctx, std::move(call_args), overload_selected_ref.parameters);

    auto selected_ref = co_await *ctx.get_compiler()->lk_functum_select_function(overload_selected_ref);
    if (selected_ref.value().overload.builtin)
    {
        std::optional< vm_value > value_maybe = co_await try_gen_call_functanoid_builtin(ctx, callee, args);

        if (value_maybe.has_value())
        {
            co_return value_maybe.value();
        }
        else
        {
            throw std::logic_error("Failed to generate call for builtin function");
        }
    }

    co_return co_await gen_invoke(ctx, overload_selected_ref, std::move(args));
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_invoke(context_frame& ctx, instantiation_type const& overload_selected_ref, vm_callargs call_args)
{

    vm_invoke call;

    std::string typestr = to_string(overload_selected_ref);
    // std::cout << "gen invoke of " << typestr << std::endl;

    call.mangled_procedure_name = mangle(overload_selected_ref);
    call.functanoid = overload_selected_ref;

    call.interface = vm_procedure_interface{};
    call.interface.argument_types = overload_selected_ref.parameters;

    auto return_type = co_await *ctx.get_compiler()->lk_functanoid_return_type(overload_selected_ref);

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
        //vm_store call_result;
        //call_result.type = call.interface.return_type.value();
        //call_result.where = tempval;

        bool return_via_pointer = true;
        // TODO: Return other ways;

        call.arguments.named.insert({"RESULT", tempval});
       // call_result.what = std::move(call);
        ctx.push(call);
        ctx.set_value_alive(temp_index);
        co_return ctx.load_temporary(temp_index);
    }
    else
    {
        ctx.push(call);
        co_return void_value();
    }
}

rpnx::general_coroutine< quxlang::compiler, quxlang::vm_callargs > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_preinvoke_conversions(context_frame& ctx, vm_callargs values, quxlang::calltype const& to_types)
{
    // TODO: Add support for default parameters.
    vm_callargs result;

    std::string dbg_to_types = quxlang::to_string(to_types);

    for (auto const& [name, val] : values.named)
    {
        auto converted_value = co_await gen_implicit_conversion(ctx, val, to_types.named.at(name));
        result.named[name] = converted_value;
    }
    for (std::size_t i = 0; i < values.positional.size(); i++)
    {
        auto converted_value = co_await gen_implicit_conversion(ctx, values.positional.at(i), to_types.positional.at(i));
        result.positional.push_back(converted_value);
    }
    co_return result;
}

rpnx::general_coroutine< quxlang::compiler, std::optional< quxlang::vm_value > > quxlang::vm_procedure_from_canonical_functanoid_resolver::try_gen_call_functanoid_builtin(context_frame& ctx, quxlang::type_symbol callee_set, vm_callargs values)
{
    assert(typeis< instantiation_type >(callee_set));

    auto callee = as< instantiation_type >(callee_set).callee;

    auto functum_ref = as< selection_reference >(callee).templexoid;

    std::string dbg_callee = to_string(callee);

    if (typeis< submember >(functum_ref))
    {
        submember const& subdot = as< submember >(functum_ref);
        type_symbol parent_type = subdot.of;

        // assert(!values.empty());

        if (subdot.name.starts_with("OPERATOR") && typeis< int_type >(parent_type))
        {
            int_type const& v_int_type = as< int_type >(parent_type);

            // if (values.size() != 2)
            // {
            //     throw std::logic_error("Invalid number of arguments to integer operator");
            //}

            vm_value lhs = values.named.at("THIS");
            vm_value rhs = values.positional.at(0);
            // TODO: Use "other" instead?

            bool is_rhs = false;

            std::string operator_str = subdot.name.substr(8);
            if (operator_str.ends_with("RHS"))
            {
                is_rhs = true;
                operator_str = operator_str.substr(0, operator_str.size() - 3);
                std::swap(lhs, rhs);
            }

            type_symbol lhs_type = vm_value_type(lhs);
            type_symbol rhs_type = vm_value_type(rhs);

            if (!assignment_operators.contains(operator_str))
            {
                assert(typeis< int_type >(lhs_type));
            }
            else
            {
                assert(typeis< int_type >(remove_ref(lhs_type)));
            }

            assert(typeis< int_type >(rhs_type));

            vm_expr_primitive_binary_op op;
            op.oper = operator_str;
            op.type = v_int_type;
            op.lhs = lhs;
            op.rhs = rhs;
            co_return op;
        }
        if (subdot.name == "CONSTRUCTOR" && typeis< int_type >(parent_type))
        {
            int_type const& v_int_type = as< int_type >(parent_type);

            // Can't call this... not possible
            // if (values.empty())
            //{
            //    throw std::logic_error("Cannot call member function with no parameters (requires at least 'this' parameter)");
            //}
            // TODO: Make asserts

            // if (values.size() > 2)
            //{
            //     throw std::logic_error("Invalid number of arguments to integer constructor");
            // }

            vm_value arg = values.named.at("THIS");

            type_symbol arg_type = vm_value_type(arg);

            if (!typeis< mvalue_reference >(arg_type) || !typeis< int_type >(remove_ref(arg_type)))
            {
                throw std::logic_error("Invalid argument type to integer constructor");
            }

            auto int_arg_type = as< int_type >(remove_ref(arg_type));
            if (int_arg_type != v_int_type)
            {
                throw std::logic_error("Unimplemented integer of different type passed to int constructor");
            }

            if (values.named.size() == 1 && values.positional.empty())
            {
                // default constructor
                vm_expr_store initalizer;

                initalizer.what = vm_expr_load_literal{"0", v_int_type};
                initalizer.where = arg;
                initalizer.type = v_int_type;
                ctx.push(vm_execute_expression{initalizer});
                co_return void_value{};
            }

            else if (values.named.size() == 1 && values.positional.size() == 1)
            {
                // copy constructor
                vm_expr_store initalizer;
                vm_value arg_to_copy = values.positional.at(0);
                type_symbol arg_copy_type = vm_value_type(arg_to_copy);
                // TODO: conversion to integer?
                if (arg_copy_type != remove_ref(arg_type))
                {
                    throw std::logic_error("Unimplemented integer of different type passed to int constructor");
                }

                initalizer.what = arg_to_copy;
                initalizer.where = arg;
                initalizer.type = v_int_type;
                ctx.push(vm_execute_expression{initalizer});
                co_return void_value{};
            }
        }
        else if (subdot.name == "CONSTRUCTOR" && values.named.size() == 1 && values.positional.size() == 0)
        {
            // For non-primitives, we should generate a default constructor if no .CONSTRUCTOR exists for the given type
            auto should_autogen = co_await *ctx.get_compiler()->lk_class_should_autogen_default_constructor(parent_type);

            if (!should_autogen)
            {
                throw std::logic_error("Not a builtin function");
            }

            co_return co_await gen_default_constructor(ctx, parent_type, values);
        }
        else if (subdot.name == "DESTRUCTOR")
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

            // assert(!is_ref(parent_type));

            co_return co_await gen_default_destructor(ctx, parent_type, values);
        }
    }

    throw std::logic_error("Unimplemented builtin function");
}

quxlang::vm_value quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_conversion_to_integer(context_frame& ctx, quxlang::vm_expr_literal val, quxlang::int_type to_type)
{
    vm_expr_load_literal result = vm_expr_load_literal{val.literal, to_type};

    return result;
}

rpnx::general_coroutine< compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build(context_frame& ctx, quxlang::function_block statement)
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

rpnx::general_coroutine< compiler, vm_value > quxlang::vm_procedure_from_canonical_functanoid_resolver::gen_default_destructor(context_frame& ctx, quxlang::type_symbol type, vm_callargs values)
{
    // TODO: make default constructing references an error
    std::string typestr = to_string(type);
    // assert(!is_ref(type));

    if (values.named.size() != 1)
    {
        throw std::logic_error("Invalid number of arguments to default constructor");
    }

    auto arg_type = vm_value_type(values.named.at("THIS"));
    assert(arg_type == make_mref(type) || arg_type == make_wref(type));

    if (is_ptr(type) || is_primitive(type) || is_ref(type))
    {
        vm_expr_store set_poison;
        set_poison.type = type;
        set_poison.where = values.named.at("THIS");
        set_poison.what = vm_expr_poison{type};
        ctx.push(vm_execute_expression{set_poison});

        co_return void_value{};
    }

    class_layout layout = co_await *ctx.get_compiler()->lk_class_layout(type);

    vm_value this_obj = values.positional.at(0);

    for (auto const& field : layout.fields)
    {
        vm_expr_access_field access;

        std::string field_typestr = to_string(field.type);
        access.type = make_mref(field.type);
        access.base = this_obj;
        access.offset = field.offset;

        auto field_destructor = submember{field.type, "DESTRUCTOR"};

        co_await gen_call(ctx, field_destructor, {.named = {{"THIS", access}}});
    }

    co_return void_value();
}

rpnx::general_coroutine< quxlang::compiler, void > quxlang::vm_procedure_from_canonical_functanoid_resolver::build_generic(quxlang::vm_procedure_from_canonical_functanoid_resolver::context_frame& ctx, quxlang::function_statement statement)
{

    if (typeis< function_var_statement >(statement))
    {
        function_var_statement var_stmt = as< function_var_statement >(statement);
        co_await build(ctx, var_stmt);
        co_return;
    }
    else if (typeis< function_expression_statement >(statement))
    {
        function_expression_statement expr_stmt = as< function_expression_statement >(statement);
        co_await build(ctx, expr_stmt);
        co_return;
    }
    else if (typeis< function_if_statement >(statement))
    {
        function_if_statement if_stmt = as< function_if_statement >(statement);
        co_await build(ctx, if_stmt);
        co_return;
    }
    else if (typeis< function_while_statement >(statement))
    {
        function_while_statement while_stmt = as< function_while_statement >(statement);
        co_await build(ctx, while_stmt);
        co_return;
    }
    else if (typeis< function_return_statement >(statement))
    {
        function_return_statement return_stmt = as< function_return_statement >(statement);
        co_await build(ctx, return_stmt);
        co_return;
    }
    else if (typeis< function_block >(statement))
    {
        function_block block_stmt = as< function_block >(statement);
        context_frame block_ctx(ctx);
        co_await build(ctx, block_stmt);
        co_await block_ctx.close();
        co_return;
    }
    else
    {
        throw std::logic_error("Unknown function statement type");
    }
    throw std::logic_error("unimplemented");
}

rpnx::general_coroutine< compiler, vm_value > vm_procedure_from_canonical_functanoid_resolver::gen_value(vm_procedure_from_canonical_functanoid_resolver::context_frame& ctx, expression_dotreference expr)
{
    auto lhsval = co_await gen_value_generic(ctx, expr.lhs);
    co_return co_await gen_access_field(ctx, lhsval, expr.field_name);
}
