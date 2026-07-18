// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/source_index.hpp"

#include <exception>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using constexpr_eval_coroutine = rpnx::querygraph::coroutine< quxlang::constexpr_eval_spec >;

    auto add_type_for_layout_scan(std::vector< quxlang::type_symbol >& pending, quxlang::type_symbol type) -> void
    {
        if (type.type_is< quxlang::nvalue_slot >())
        {
            pending.push_back(type.get_as< quxlang::nvalue_slot >().target);
            return;
        }
        if (type.type_is< quxlang::dvalue_slot >())
        {
            pending.push_back(type.get_as< quxlang::dvalue_slot >().target);
            return;
        }
        if (type.type_is< quxlang::ptrref_type >())
        {
            pending.push_back(type.get_as< quxlang::ptrref_type >().target);
            return;
        }
        if (type.type_is< quxlang::array_type >())
        {
            pending.push_back(type.get_as< quxlang::array_type >().element_type);
            return;
        }
        if (type.type_is< quxlang::storage >())
        {
            for (auto const& storable_type : type.get_as< quxlang::storage >().storable_types)
            {
                pending.push_back(storable_type);
            }
            return;
        }
    }

    auto type_might_have_layout(quxlang::type_symbol const& type) -> bool
    {
        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::subtag_type >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >();
    }
} // namespace

rpnx::querygraph::coroutine< quxlang::constexpr_eval_spec > quxlang::constexpr_eval_impl(constexpr_input2 input)
{
    vmir2::ir2_constexpr_interpreter interp;
    [[maybe_unused]] std::vector< std::string > debug_lines;
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        interp.set_debug_line_handler([&debug_lines](std::string line)
                                      {
                                          debug_lines.push_back(std::move(line));
                                      });
    }
    auto source_file_index = co_await rpnx::querygraph::request< source_file_index_query >(std::monostate{});
    auto source_bundle = co_await rpnx::querygraph::request< source_bundle_query >(std::monostate{});
    interp.set_source_index(vmir2::source_index(source_file_index, source_bundle));

    std::set< type_symbol > layout_types;
    std::set< type_symbol > loaded_layouts;
    std::vector< instanciation_reference > pending_functanoids;
    std::set< type_symbol > queued_functanoids;
    std::set< type_symbol > loaded_functanoids;
    std::set< type_symbol > asm_procedure_symbols;
    std::vector< type_symbol > pending_antestatal_globals;
    std::set< type_symbol > queued_antestatal_globals;
    std::set< type_symbol > loaded_antestatal_globals;

    auto enqueue_functanoid = [&](type_symbol const& funcname)
    {
        if (!typeis< instanciation_reference >(funcname))
        {
            throw compiler_bug("functanoid dependency is not an instanciation reference: " + to_string(funcname));
        }
        if (loaded_functanoids.contains(funcname) || !queued_functanoids.insert(funcname).second)
        {
            return;
        }
        pending_functanoids.push_back(funcname.get_as< instanciation_reference >());
    };

    auto enqueue_functanoids = [&](std::set< type_symbol > const& functanoids)
    {
        for (type_symbol const& funcname : functanoids)
        {
            enqueue_functanoid(funcname);
        }
    };

    auto enqueue_antestatal_global = [&](type_symbol const& symbol)
    {
        if (input.antestatal_global_symbol.has_value() && *input.antestatal_global_symbol == symbol)
        {
            return;
        }
        if (loaded_antestatal_globals.contains(symbol) || !queued_antestatal_globals.insert(symbol).second)
        {
            return;
        }
        pending_antestatal_globals.push_back(symbol);
    };

    auto enqueue_antestatal_globals = [&](std::set< type_symbol > const& symbols)
    {
        for (type_symbol const& symbol : symbols)
        {
            enqueue_antestatal_global(symbol);
        }
    };

    auto enqueue_layouts = [&](std::set< type_symbol > const& types)
    {
        for (type_symbol const& type : types)
        {
            layout_types.insert(type);
        }
    };

    auto add_queued_layouts = [&]() -> constexpr_eval_coroutine::cosubroutine< void >
    {
        for (type_symbol const& root_type : layout_types)
        {
            std::vector< type_symbol > pending{root_type};

            while (!pending.empty())
            {
                type_symbol type = std::move(pending.back());
                pending.pop_back();

                add_type_for_layout_scan(pending, type);

                if (!type_might_have_layout(type) || loaded_layouts.contains(type))
                {
                    continue;
                }

                loaded_layouts.insert(type);
                class_kind const kind = co_await rpnx::querygraph::request< class_type_query >(type);
                if (kind == class_kind::enum_)
                {
                    enum_info const info = co_await rpnx::querygraph::request< enum_info_query >(type);
                    interp.add_enum_info(type, info);
                    continue;
                }
                if (kind == class_kind::flagset)
                {
                    flagset_info const info = co_await rpnx::querygraph::request< flagset_info_query >(type);
                    interp.add_nominal_integer_type(type, info.bits);
                    continue;
                }
                if (kind == class_kind::union_)
                {
                    union_info const info = co_await rpnx::querygraph::request< union_info_query >(type);
                    fusion_layout const layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    interp.add_union_info(type, info);
                    interp.add_fusion_layout(type, layout);
                    for (union_option_info const& option : info.options)
                    {
                        pending.push_back(option.type);
                    }
                    continue;
                }
                if (kind == class_kind::variant)
                {
                    variant_info const info = co_await rpnx::querygraph::request< variant_info_query >(type);
                    fusion_layout const layout = co_await rpnx::querygraph::request< fusion_layout_query >(type);
                    interp.add_variant_info(type, info);
                    interp.add_fusion_layout(type, layout);
                    for (type_symbol const& alternative : info.alternatives)
                    {
                        pending.push_back(alternative);
                    }
                    continue;
                }
                if (kind != class_kind::struct_)
                {
                    continue;
                }

                struct_layout const layout = co_await rpnx::querygraph::request< struct_layout_query >(type);
                interp.add_struct_layout(type, layout);
                for (auto const& field : layout.fields)
                {
                    pending.push_back(field.type);
                }
            }
        }
    };

    auto provide_antestatal_global = [&](type_symbol const& symbol) -> constexpr_eval_coroutine::cosubroutine< std::pair< std::set< type_symbol >, std::set< type_symbol > > >
    {
        if (!(co_await rpnx::querygraph::request< global_is_antestatal_static_query >(symbol)))
        {
            throw compiler_bug("constexpr interpreter requested non-antestatal global data: " + to_string(symbol));
        }

        type_symbol type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
        layout_types.insert(type);
        antestatal_value value = co_await rpnx::querygraph::request< antestatal_static_value_query >(symbol);
        dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
            direct_dependencies_input{.symbol = symbol, .set = dependency_set::constexpr_});
        std::set< type_symbol > value_functanoids;
        for (auto const& [functanoid, _] : dependencies.functanoids) value_functanoids.insert(functanoid);
        std::set< type_symbol > value_antestatal_globals = dependencies.antestatal_globals;
        interp.add_constexpr_antestatal_global(symbol, type, std::move(value));
        co_return std::pair(std::move(value_functanoids), std::move(value_antestatal_globals));
    };

    auto add_zero_initialized_global_storages = [&](dependencies const& dependencies) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        for (type_symbol const& symbol : dependencies.global_roots)
        {
            initialization_type const init_type = co_await rpnx::querygraph::request< global_init_type_query >(symbol);
            if (init_type != initialization_type::init_trivial)
            {
                continue;
            }

            type_symbol type = co_await rpnx::querygraph::request< variable_type_query >(symbol);
            layout_types.insert(type);
            interp.add_zero_initialized_global(symbol, type);
        }
    };

    constexpr_routine_result const& routine_result = co_await rpnx::querygraph::request< constexpr_routine_query >(input);
    vmir2::functanoid_routine3 const& ir3 = routine_result.routine;
    dependencies const& root_dependencies = routine_result.direct_dependencies;
    layout_types.insert(input.type);
    enqueue_layouts(root_dependencies.struct_layouts);
    enqueue_layouts(root_dependencies.fusion_layouts);
    for (auto const& [functanoid, _] : root_dependencies.functanoids) enqueue_functanoid(functanoid);
    enqueue_antestatal_globals(root_dependencies.antestatal_globals);
    co_await add_zero_initialized_global_storages(root_dependencies);

    interp.add_functanoid3(void_type{}, ir3, root_dependencies.static_snapshots);
    loaded_functanoids.insert(type_symbol(void_type{}));

    while (!pending_functanoids.empty() || !pending_antestatal_globals.empty())
    {
        std::vector< type_symbol > round_antestatal_globals;
        while (!pending_antestatal_globals.empty())
        {
            type_symbol symbol = std::move(pending_antestatal_globals.back());
            pending_antestatal_globals.pop_back();
            if (loaded_antestatal_globals.contains(symbol))
            {
                continue;
            }
            round_antestatal_globals.push_back(std::move(symbol));
        }

        for (type_symbol const& symbol : round_antestatal_globals)
        {
            co_yield rpnx::querygraph::dependency< global_is_antestatal_static_query >(rpnx::querygraph::request< global_is_antestatal_static_query >(symbol));
            co_yield rpnx::querygraph::dependency< variable_type_query >(rpnx::querygraph::request< variable_type_query >(symbol));
            co_yield rpnx::querygraph::dependency< antestatal_static_value_query >(rpnx::querygraph::request< antestatal_static_value_query >(symbol));
        }

        std::vector< instanciation_reference > round_functanoids;
        while (!pending_functanoids.empty())
        {
            instanciation_reference functanoid = std::move(pending_functanoids.back());
            pending_functanoids.pop_back();

            type_symbol funcname = functanoid;
            if (loaded_functanoids.contains(funcname))
            {
                continue;
            }
            round_functanoids.push_back(std::move(functanoid));
        }

        std::vector< instanciation_reference > round_functions;
        for (instanciation_reference const& invocable : round_functanoids)
        {
            type_symbol const symbol = invocable;
            ast2_symboid const& symboid = co_await rpnx::querygraph::request< symboid_query >(invocable.temploid.templexoid);
            if (typeis< ast2_asm_procedure_declaration >(symboid) || typeis< ast2_extern_procedure >(symboid))
            {
                if (asm_procedure_symbols.insert(symbol).second)
                {
                    interp.add_constexpr_asm_procedure(symbol);
                }
                loaded_functanoids.insert(symbol);
                continue;
            }

            round_functions.push_back(invocable);
        }

        for (instanciation_reference const& functanoid : round_functions)
        {
            co_yield rpnx::querygraph::dependency< vm_procedure3_query >(rpnx::querygraph::request< vm_procedure3_query >(functanoid));
        }

        for (type_symbol const& symbol : round_antestatal_globals)
        {
            if (loaded_antestatal_globals.contains(symbol))
            {
                continue;
            }

            std::pair< std::set< type_symbol >, std::set< type_symbol > > requirements = co_await provide_antestatal_global(symbol);
            loaded_antestatal_globals.insert(symbol);
            enqueue_functanoids(requirements.first);
            enqueue_antestatal_globals(requirements.second);
        }

        for (instanciation_reference const& functanoid : round_functions)
        {
            type_symbol funcname = functanoid;
            if (loaded_functanoids.contains(funcname))
            {
                continue;
            }

            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
            dependencies const& dependencies = co_await rpnx::querygraph::request< direct_dependencies_query >(
                direct_dependencies_input{.symbol = funcname, .set = dependency_set::constexpr_});
            interp.add_functanoid3(funcname, ir2_other, dependencies.static_snapshots);
            loaded_functanoids.insert(funcname);
            enqueue_layouts(dependencies.struct_layouts);
            enqueue_layouts(dependencies.fusion_layouts);
            enqueue_antestatal_globals(dependencies.antestatal_globals);
            co_await add_zero_initialized_global_storages(dependencies);
            for (auto const& [dependency, _] : dependencies.functanoids) enqueue_functanoid(dependency);
        }
    }

    co_await add_queued_layouts();

    std::exception_ptr execution_exception;
    try
    {
        interp.exec3(void_type{});
    }
    catch (...)
    {
        execution_exception = std::current_exception();
    }
    if constexpr (QUXLANG_DEBUG_MESSAGES_ENABLED)
    {
        for (std::string const& debug_line : debug_lines)
        {
            co_yield rpnx::querygraph::debug_message("{}", debug_line);
        }
    }
    if (execution_exception)
    {
        std::rethrow_exception(execution_exception);
    }

    auto val = interp.get_cr_value();
    val.type = type_symbol(bool_type{});
    co_return val;
}
