// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/data/compilation_result.hpp>
#include <quxlang/queries/specs/constexpr_eval_spec.hpp>
#include "quxlang/bytemath.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/ir2_constexpr_interpreter.hpp"
#include "quxlang/vmir2/routine_requirements.hpp"
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
        return type.type_is< quxlang::subsymbol >() || type.type_is< quxlang::instanciation_reference >() || type.type_is< quxlang::readonly_constant >();
    }

    struct constexpr_eval_dependency_provider
    {
        std::set< quxlang::type_symbol > layout_types;

        auto add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> constexpr_eval_coroutine::cosubroutine< void >;
        auto add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol result_type, quxlang::vmir2::functanoid_routine3 const& routine) -> constexpr_eval_coroutine::cosubroutine< void >;
        auto provide_antestatal_global(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol const& symbol)
            -> constexpr_eval_coroutine::cosubroutine< std::pair< std::set< quxlang::type_symbol >, std::set< quxlang::type_symbol > > >;
    };

    auto constexpr_eval_dependency_provider::add_layouts_for_type(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol type) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        std::vector< quxlang::type_symbol > pending{std::move(type)};

        while (!pending.empty())
        {
            auto type = pending.back();
            pending.pop_back();

            add_type_for_layout_scan(pending, type);

            if (!type_might_have_layout(type) || layout_types.contains(type))
            {
                continue;
            }

            layout_types.insert(type);
            quxlang::symbol_kind const kind = co_await rpnx::querygraph::request< quxlang::symbol_type_query >(type);
            if (kind == quxlang::symbol_kind::enum_)
            {
                quxlang::enum_info const info = co_await rpnx::querygraph::request< quxlang::enum_info_query >(type);
                interp.add_nominal_integer_type(type, info.bits);
                continue;
            }
            if (kind == quxlang::symbol_kind::flagset_)
            {
                quxlang::flagset_info const info = co_await rpnx::querygraph::request< quxlang::flagset_info_query >(type);
                interp.add_nominal_integer_type(type, info.bits);
                continue;
            }
            auto layout = co_await rpnx::querygraph::request< quxlang::class_layout_query >(type);
            interp.add_class_layout(type, layout);
            for (auto const& field : layout.fields)
            {
                pending.push_back(field.type);
            }
        }
    }

    auto constexpr_eval_dependency_provider::add_layouts_for_routine(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol result_type, quxlang::vmir2::functanoid_routine3 const& routine) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        co_await add_layouts_for_type(interp, result_type);
        for (auto const& local_type : routine.local_types)
        {
            co_await add_layouts_for_type(interp, local_type.type);
        }
        for (auto const& [_, param] : routine.parameters.named)
        {
            co_await add_layouts_for_type(interp, param.type);
        }
        for (auto const& param : routine.parameters.positional)
        {
            co_await add_layouts_for_type(interp, param.type);
        }
    }

    auto constexpr_eval_dependency_provider::provide_antestatal_global(quxlang::vmir2::ir2_constexpr_interpreter& interp, quxlang::type_symbol const& symbol)
        -> constexpr_eval_coroutine::cosubroutine< std::pair< std::set< quxlang::type_symbol >, std::set< quxlang::type_symbol > > >
    {
        if (!(co_await rpnx::querygraph::request< quxlang::global_is_antestatal_static_query >(symbol)))
        {
            throw quxlang::compiler_bug("constexpr interpreter requested non-antestatal global data: " + quxlang::to_string(symbol));
        }

        quxlang::type_symbol type = co_await rpnx::querygraph::request< quxlang::variable_type_query >(symbol);
        co_await add_layouts_for_type(interp, type);
        quxlang::antestatal_value value = co_await rpnx::querygraph::request< quxlang::antestatal_static_value_query >(symbol);
        std::set< quxlang::type_symbol > value_functanoids = quxlang::vmir2::directly_instantiated_functanoids(value, type);
        std::set< quxlang::type_symbol > value_antestatal_globals = quxlang::vmir2::directly_referenced_antestatal_globals(value, type);
        interp.add_constexpr_antestatal_global(symbol, type, std::move(value));
        co_return std::pair(std::move(value_functanoids), std::move(value_antestatal_globals));
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

    constexpr_eval_dependency_provider dependency_provider;
    std::vector< instanciation_reference > pending_functanoids;
    std::set< type_symbol > queued_functanoids;
    std::set< type_symbol > loaded_functanoids;
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

    auto add_layouts_for_functanoid = [&](instanciation_reference const& functanoid) -> constexpr_eval_coroutine::cosubroutine< void >
    {
        std::set< type_symbol > const required_layouts = co_await rpnx::querygraph::request< functanoid_required_class_layouts_query >(
            functanoid_requirement_input{.functanoid = functanoid, .compilation_type = functanoid_compilation_type::all});
        for (type_symbol const& type : required_layouts)
        {
            co_await dependency_provider.add_layouts_for_type(interp, type);
        }
    };

    auto ir3 = co_await rpnx::querygraph::request< constexpr_routine_query >(input);
    co_await dependency_provider.add_layouts_for_routine(interp, input.type, ir3);
    enqueue_functanoids(vmir2::directly_instantiated_functanoids(ir3));
    enqueue_antestatal_globals(vmir2::directly_referenced_antestatal_globals(ir3));

    interp.add_functanoid3(void_type{}, ir3);
    loaded_functanoids.insert(type_symbol(void_type{}));

    while (!pending_functanoids.empty() || !pending_antestatal_globals.empty())
    {
        while (!pending_antestatal_globals.empty())
        {
            type_symbol symbol = std::move(pending_antestatal_globals.back());
            pending_antestatal_globals.pop_back();
            if (loaded_antestatal_globals.contains(symbol))
            {
                continue;
            }

            std::pair< std::set< type_symbol >, std::set< type_symbol > > requirements = co_await dependency_provider.provide_antestatal_global(interp, symbol);
            loaded_antestatal_globals.insert(symbol);
            enqueue_functanoids(requirements.first);
            enqueue_antestatal_globals(requirements.second);
        }

        while (!pending_functanoids.empty())
        {
            instanciation_reference functanoid = std::move(pending_functanoids.back());
            pending_functanoids.pop_back();

            type_symbol funcname = functanoid;
            if (loaded_functanoids.contains(funcname))
            {
                continue;
            }

            co_await add_layouts_for_functanoid(functanoid);
            vmir2::functanoid_routine3 const& ir2_other = co_await rpnx::querygraph::request< vm_procedure3_query >(functanoid);
            interp.add_functanoid3(funcname, ir2_other);
            loaded_functanoids.insert(funcname);
            enqueue_antestatal_globals(vmir2::directly_referenced_antestatal_globals(ir2_other));

            std::set< type_symbol > const direct_functanoids = co_await rpnx::querygraph::request< functanoid_directly_instantiated_functanoids_query >(
                functanoid_requirement_input{.functanoid = functanoid, .compilation_type = functanoid_compilation_type::all});
            enqueue_functanoids(direct_functanoids);
        }
    }

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
